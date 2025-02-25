// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "iosconfigurations.h"
#include "iosdevice.h"
#include "iosrunconfiguration.h"
#include "iosrunner.h"
#include "iossimulator.h"
#include "iosconstants.h"

#include <debugger/debuggerplugin.h>
#include <debugger/debuggerkitinformation.h>

#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/taskhub.h>
#include <projectexplorer/toolchain.h>

#include <qmldebug/qmldebugcommandlinearguments.h>

#include <utils/fileutils.h>
#include <utils/qtcprocess.h>
#include <utils/url.h>
#include <utils/utilsicons.h>

#include <QDateTime>
#include <QDir>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSettings>
#include <QTcpServer>
#include <QTime>

#include <stdio.h>
#include <fcntl.h>
#ifdef Q_OS_UNIX
#include <unistd.h>
#else
#include <io.h>
#endif
#include <signal.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace Ios {
namespace Internal {

static void stopRunningRunControl(RunControl *runControl)
{
    static QMap<Utils::Id, QPointer<RunControl>> activeRunControls;

    Target *target = runControl->target();
    Utils::Id devId = DeviceKitAspect::deviceId(target->kit());

    // The device can only run an application at a time, if an app is running stop it.
    if (activeRunControls.contains(devId)) {
        if (QPointer<RunControl> activeRunControl = activeRunControls[devId])
            activeRunControl->initiateStop();
        activeRunControls.remove(devId);
    }

    if (devId.isValid())
        activeRunControls[devId] = runControl;
}

IosRunner::IosRunner(RunControl *runControl)
    : RunWorker(runControl)
{
    setId("IosRunner");
    stopRunningRunControl(runControl);
    const IosDeviceTypeAspect::Data *data = runControl->aspect<IosDeviceTypeAspect>();
    QTC_ASSERT(data, return);
    m_bundleDir = data->bundleDirectory.toString();
    m_device = DeviceKitAspect::device(runControl->kit());
    m_deviceType = data->deviceType;
}

IosRunner::~IosRunner()
{
    stop();
}

void IosRunner::setCppDebugging(bool cppDebug)
{
    m_cppDebug = cppDebug;
}

void IosRunner::setQmlDebugging(QmlDebug::QmlDebugServicesPreset qmlDebugServices)
{
    m_qmlDebugServices = qmlDebugServices;
}

QString IosRunner::bundlePath()
{
    return m_bundleDir;
}

QString IosRunner::deviceId()
{
    IosDevice::ConstPtr dev = m_device.dynamicCast<const IosDevice>();
    if (!dev)
        return QString();
    return dev->uniqueDeviceID();
}

IosToolHandler::RunKind IosRunner::runType()
{
    if (m_cppDebug)
        return IosToolHandler::DebugRun;
    return IosToolHandler::NormalRun;
}

bool IosRunner::cppDebug() const
{
    return m_cppDebug;
}

bool IosRunner::qmlDebug() const
{
    return m_qmlDebugServices != QmlDebug::NoQmlDebugServices;
}

QmlDebug::QmlDebugServicesPreset IosRunner::qmlDebugServices() const
{
    return m_qmlDebugServices;
}

void IosRunner::start()
{
    if (m_toolHandler && isAppRunning())
        m_toolHandler->stop();

    m_cleanExit = false;
    m_qmlServerPort = Port();
    if (!QFileInfo::exists(m_bundleDir)) {
        TaskHub::addTask(DeploymentTask(Task::Warning, tr("Could not find %1.").arg(m_bundleDir)));
        reportFailure();
        return;
    }
    if (m_device->type() == Ios::Constants::IOS_DEVICE_TYPE) {
        IosDevice::ConstPtr iosDevice = m_device.dynamicCast<const IosDevice>();
        if (m_device.isNull()) {
            reportFailure();
            return;
        }
        if (m_qmlDebugServices != QmlDebug::NoQmlDebugServices)
            m_qmlServerPort = iosDevice->nextPort();
    } else {
        IosSimulator::ConstPtr sim = m_device.dynamicCast<const IosSimulator>();
        if (sim.isNull()) {
            reportFailure();
            return;
        }
        if (m_qmlDebugServices != QmlDebug::NoQmlDebugServices)
            m_qmlServerPort = sim->nextPort();
    }

    m_toolHandler = new IosToolHandler(m_deviceType, this);
    connect(m_toolHandler, &IosToolHandler::appOutput,
            this, &IosRunner::handleAppOutput);
    connect(m_toolHandler, &IosToolHandler::errorMsg,
            this, &IosRunner::handleErrorMsg);
    connect(m_toolHandler, &IosToolHandler::gotServerPorts,
            this, &IosRunner::handleGotServerPorts);
    connect(m_toolHandler, &IosToolHandler::gotInferiorPid,
            this, &IosRunner::handleGotInferiorPid);
    connect(m_toolHandler, &IosToolHandler::toolExited,
            this, &IosRunner::handleToolExited);
    connect(m_toolHandler, &IosToolHandler::finished,
            this, &IosRunner::handleFinished);

    const CommandLine command = runControl()->commandLine();
    QStringList args = ProcessArgs::splitArgs(command.arguments(), OsTypeMac);
    if (m_qmlServerPort.isValid()) {
        QUrl qmlServer;
        qmlServer.setPort(m_qmlServerPort.number());
        args.append(QmlDebug::qmlDebugTcpArguments(m_qmlDebugServices, qmlServer));
    }

    m_toolHandler->requestRunApp(bundlePath(), args, runType(), deviceId());
}

void IosRunner::stop()
{
    if (isAppRunning())
        m_toolHandler->stop();
}

void IosRunner::handleGotServerPorts(IosToolHandler *handler, const QString &bundlePath,
                                     const QString &deviceId, Port gdbPort,
                                     Port qmlPort)
{
    // Called when debugging on Device.
    Q_UNUSED(bundlePath)
    Q_UNUSED(deviceId)

    if (m_toolHandler != handler)
        return;

    m_gdbServerPort = gdbPort;
    m_qmlServerPort = qmlPort;

    bool prerequisiteOk = false;
    if (cppDebug() && qmlDebug())
        prerequisiteOk = m_gdbServerPort.isValid() && m_qmlServerPort.isValid();
    else if (cppDebug())
        prerequisiteOk = m_gdbServerPort.isValid();
    else if (qmlDebug())
        prerequisiteOk = m_qmlServerPort.isValid();
    else
        prerequisiteOk = true; // Not debugging. Ports not required.


    if (prerequisiteOk)
        reportStarted();
    else
        reportFailure(tr("Could not get necessary ports for the debugger connection."));
}

void IosRunner::handleGotInferiorPid(IosToolHandler *handler, const QString &bundlePath,
                                     const QString &deviceId, qint64 pid)
{
    // Called when debugging on Simulator.
    Q_UNUSED(bundlePath)
    Q_UNUSED(deviceId)

    if (m_toolHandler != handler)
        return;

    m_pid = pid;
    bool prerequisiteOk = false;
    if (m_pid > 0) {
        prerequisiteOk = true;
    } else {
        reportFailure(tr("Could not get inferior PID."));
        return;
    }

    if (qmlDebug())
        prerequisiteOk = m_qmlServerPort.isValid();

    if (prerequisiteOk)
        reportStarted();
    else
        reportFailure(tr("Could not get necessary ports for the debugger connection."));
}

void IosRunner::handleAppOutput(IosToolHandler *handler, const QString &output)
{
    Q_UNUSED(handler)
    QRegularExpression qmlPortRe("QML Debugger: Waiting for connection on port ([0-9]+)...");
    const QRegularExpressionMatch match = qmlPortRe.match(output);
    QString res(output);
    if (match.hasMatch() && m_qmlServerPort.isValid())
       res.replace(match.captured(1), QString::number(m_qmlServerPort.number()));
    appendMessage(output, StdOutFormat);
    appOutput(res);
}

void IosRunner::handleErrorMsg(IosToolHandler *handler, const QString &msg)
{
    Q_UNUSED(handler)
    QString res(msg);
    QString lockedErr ="Unexpected reply: ELocked (454c6f636b6564) vs OK (4f4b)";
    if (msg.contains("AMDeviceStartService returned -402653150")) {
        TaskHub::addTask(DeploymentTask(Task::Warning, tr("Run failed. "
           "The settings in the Organizer window of Xcode might be incorrect.")));
    } else if (res.contains(lockedErr)) {
        QString message = tr("The device is locked, please unlock.");
        TaskHub::addTask(DeploymentTask(Task::Error, message));
        res.replace(lockedErr, message);
    }
    QRegularExpression qmlPortRe("QML Debugger: Waiting for connection on port ([0-9]+)...");
    const QRegularExpressionMatch match = qmlPortRe.match(msg);
    if (match.hasMatch() && m_qmlServerPort.isValid())
       res.replace(match.captured(1), QString::number(m_qmlServerPort.number()));

    appendMessage(res, StdErrFormat);
    errorMsg(res);
}

void IosRunner::handleToolExited(IosToolHandler *handler, int code)
{
    Q_UNUSED(handler)
    m_cleanExit = (code == 0);
}

void IosRunner::handleFinished(IosToolHandler *handler)
{
    if (m_toolHandler == handler) {
        if (m_cleanExit)
            appendMessage(tr("Run ended."), NormalMessageFormat);
        else
            appendMessage(tr("Run ended with error."), ErrorMessageFormat);
        m_toolHandler = nullptr;
    }
    handler->deleteLater();
    reportStopped();
}

qint64 IosRunner::pid() const
{
    return m_pid;
}

bool IosRunner::isAppRunning() const
{
    return m_toolHandler && m_toolHandler->isRunning();
}

Utils::Port IosRunner::gdbServerPort() const
{
    return m_gdbServerPort;
}

Utils::Port IosRunner::qmlServerPort() const
{
    return m_qmlServerPort;
}

//
// IosRunner
//

IosRunSupport::IosRunSupport(RunControl *runControl)
    : IosRunner(runControl)
{
    setId("IosRunSupport");
    runControl->setIcon(Icons::RUN_SMALL_TOOLBAR);
    QString displayName = QString("Run on %1").arg(device().isNull() ? QString() : device()->displayName());
    runControl->setDisplayName(displayName);
}

IosRunSupport::~IosRunSupport()
{
    stop();
}

void IosRunSupport::start()
{
    appendMessage(tr("Starting remote process."), NormalMessageFormat);
    IosRunner::start();
}

//
// IosQmlProfilerSupport
//

IosQmlProfilerSupport::IosQmlProfilerSupport(RunControl *runControl)
    : RunWorker(runControl)
{
    setId("IosQmlProfilerSupport");

    m_runner = new IosRunner(runControl);
    m_runner->setQmlDebugging(QmlDebug::QmlProfilerServices);
    addStartDependency(m_runner);

    m_profiler = runControl->createWorker(ProjectExplorer::Constants::QML_PROFILER_RUNNER);
    m_profiler->addStartDependency(this);
}

void IosQmlProfilerSupport::start()
{
    QUrl serverUrl;
    QTcpServer server;
    QTC_ASSERT(server.listen(QHostAddress::LocalHost)
               || server.listen(QHostAddress::LocalHostIPv6), return);
    serverUrl.setScheme(Utils::urlTcpScheme());
    serverUrl.setHost(server.serverAddress().toString());

    Port qmlPort = m_runner->qmlServerPort();
    serverUrl.setPort(qmlPort.number());
    m_profiler->recordData("QmlServerUrl", serverUrl);
    if (qmlPort.isValid())
        reportStarted();
    else
        reportFailure(tr("Could not get necessary ports for the profiler connection."));
}

//
// IosDebugSupport
//

IosDebugSupport::IosDebugSupport(RunControl *runControl)
    : Debugger::DebuggerRunTool(runControl)
{
    setId("IosDebugSupport");

    m_runner = new IosRunner(runControl);
    m_runner->setCppDebugging(isCppDebugging());
    m_runner->setQmlDebugging(isQmlDebugging() ? QmlDebug::QmlDebuggerServices : QmlDebug::NoQmlDebugServices);

    addStartDependency(m_runner);
}

void IosDebugSupport::start()
{
    if (!m_runner->isAppRunning()) {
        reportFailure(tr("Application not running."));
        return;
    }

    if (device()->type() == Ios::Constants::IOS_DEVICE_TYPE) {
        IosDevice::ConstPtr dev = device().dynamicCast<const IosDevice>();
        setStartMode(AttachToRemoteProcess);
        setIosPlatform("remote-ios");
        const QString osVersion = dev->osVersion();
        const QString cpuArchitecture = dev->cpuArchitecture();
        const FilePaths symbolsPathCandidates = {
            FilePath::fromString(QDir::homePath() + "/Library/Developer/Xcode/iOS DeviceSupport/"
                                 + osVersion + " " + cpuArchitecture + "/Symbols"),
            FilePath::fromString(QDir::homePath() + "/Library/Developer/Xcode/iOS DeviceSupport/"
                                 + osVersion + "/Symbols"),
            IosConfigurations::developerPath().pathAppended(
                "Platforms/iPhoneOS.platform/DeviceSupport/" + osVersion + "/Symbols")};
        const FilePath deviceSdk = Utils::findOrDefault(symbolsPathCandidates, &FilePath::isDir);

        if (deviceSdk.isEmpty()) {
            TaskHub::addTask(DeploymentTask(
                Task::Warning,
                tr("Could not find device specific debug symbols at %1. "
                   "Debugging initialization will be slow until you open the Organizer window of "
                   "Xcode with the device connected to have the symbols generated.")
                    .arg(symbolsPathCandidates.constFirst().toUserOutput())));
        }
        setDeviceSymbolsRoot(deviceSdk.toString());
    } else {
        setStartMode(AttachToLocalProcess);
        setIosPlatform("ios-simulator");
    }

    const IosDeviceTypeAspect::Data *data = runControl()->aspect<IosDeviceTypeAspect>();
    QTC_ASSERT(data, reportFailure("Broken IosDeviceTypeAspect setup."); return);

    setRunControlName(data->applicationName);
    setContinueAfterAttach(true);

    Utils::Port gdbServerPort = m_runner->gdbServerPort();
    Utils::Port qmlServerPort = m_runner->qmlServerPort();
    setAttachPid(ProcessHandle(m_runner->pid()));

    const bool cppDebug = isCppDebugging();
    const bool qmlDebug = isQmlDebugging();
    if (cppDebug) {
        setInferiorExecutable(data->localExecutable);
        setRemoteChannel("connect://localhost:" + gdbServerPort.toString());

        QString bundlePath = data->bundleDirectory.toString();
        bundlePath.chop(4);
        FilePath dsymPath = FilePath::fromString(bundlePath.append(".dSYM"));
        if (dsymPath.exists()
                && dsymPath.lastModified() < data->localExecutable.lastModified()) {
            TaskHub::addTask(DeploymentTask(Task::Warning,
                     tr("The dSYM %1 seems to be outdated, it might confuse the debugger.")
                             .arg(dsymPath.toUserOutput())));
        }
    }

    QUrl qmlServer;
    if (qmlDebug) {
        QTcpServer server;
        QTC_ASSERT(server.listen(QHostAddress::LocalHost)
                   || server.listen(QHostAddress::LocalHostIPv6), return);
        qmlServer.setHost(server.serverAddress().toString());
        if (!cppDebug)
            setStartMode(AttachToRemoteServer);
    }

    if (qmlServerPort.isValid())
        qmlServer.setPort(qmlServerPort.number());

    setQmlServer(qmlServer);

    DebuggerRunTool::start();
}

} // namespace Internal
} // namespace Ios
