// Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "androidavdmanager.h"
#include "androidconfigurations.h"
#include "androidconstants.h"
#include "androiddeployqtstep.h"
#include "androiddevice.h"
#include "androidmanager.h"
#include "androidrunconfiguration.h"
#include "androidrunner.h"
#include "androidrunnerworker.h"
#include "androidtr.h"

#include <coreplugin/messagemanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorersettings.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/url.h>

#include <QHostAddress>
#include <QLoggingCategory>

namespace {
static Q_LOGGING_CATEGORY(androidRunnerLog, "qtc.android.run.androidrunner", QtWarningMsg)
}

using namespace ProjectExplorer;
using namespace Utils;

namespace Android {
namespace Internal {

AndroidRunner::AndroidRunner(RunControl *runControl, const QString &intentName)
    : RunWorker(runControl), m_target(runControl->target())
{
    setId("AndroidRunner");
    static const int metaTypes[] = {
        qRegisterMetaType<QVector<QStringList> >("QVector<QStringList>"),
        qRegisterMetaType<Utils::Port>("Utils::Port"),
        qRegisterMetaType<AndroidDeviceInfo>("Android::AndroidDeviceInfo")
    };
    Q_UNUSED(metaTypes)

    m_checkAVDTimer.setInterval(2000);
    connect(&m_checkAVDTimer, &QTimer::timeout, this, &AndroidRunner::checkAVD);

    QString intent = intentName;
    if (intent.isEmpty())
        intent = AndroidManager::packageName(m_target) + '/' + AndroidManager::activityName(m_target);

    m_packageName = intent.left(intent.indexOf('/'));
    qCDebug(androidRunnerLog) << "Intent name:" << intent << "Package name" << m_packageName;

    const int apiLevel = AndroidManager::deviceApiLevel(m_target);
    qCDebug(androidRunnerLog) << "Device API:" << apiLevel;

    m_worker.reset(new AndroidRunnerWorker(this, m_packageName));
    m_worker->setIntentName(intent);
    m_worker->setIsPreNougat(apiLevel <= 23);

    m_worker->moveToThread(&m_thread);

    connect(this, &AndroidRunner::asyncStart, m_worker.data(), &AndroidRunnerWorker::asyncStart);
    connect(this, &AndroidRunner::asyncStop, m_worker.data(), &AndroidRunnerWorker::asyncStop);
    connect(this, &AndroidRunner::androidDeviceInfoChanged,
            m_worker.data(), &AndroidRunnerWorker::setAndroidDeviceInfo);
    connect(m_worker.data(), &AndroidRunnerWorker::remoteProcessStarted,
            this, &AndroidRunner::handleRemoteProcessStarted);
    connect(m_worker.data(), &AndroidRunnerWorker::remoteProcessFinished,
            this, &AndroidRunner::handleRemoteProcessFinished);
    connect(m_worker.data(), &AndroidRunnerWorker::remoteOutput,
            this, &AndroidRunner::remoteOutput);
    connect(m_worker.data(), &AndroidRunnerWorker::remoteErrorOutput,
            this, &AndroidRunner::remoteErrorOutput);

    connect(&m_outputParser, &QmlDebug::QmlOutputParser::waitingForConnectionOnPort,
            this, &AndroidRunner::qmlServerPortReady);

    m_thread.start();
}

AndroidRunner::~AndroidRunner()
{
    m_thread.quit();
    m_thread.wait();
}

void AndroidRunner::start()
{
    if (!ProjectExplorerPlugin::projectExplorerSettings().deployBeforeRun) {
        qCDebug(androidRunnerLog) << "Run without deployment";
       launchAVD();
       if (!m_launchedAVDName.isEmpty()) {
           m_checkAVDTimer.start();
           return;
       }
    }

    emit asyncStart();
}

void AndroidRunner::stop()
{
    if (m_checkAVDTimer.isActive()) {
        m_checkAVDTimer.stop();
        appendMessage("\n\n" + Tr::tr("\"%1\" terminated.").arg(m_packageName),
                      Utils::NormalMessageFormat);
        return;
    }

    emit asyncStop();
}

void AndroidRunner::qmlServerPortReady(Port port)
{
    // FIXME: Note that the passed is nonsense, as the port is on the
    // device side. It only happens to work since we redirect
    // host port n to target port n via adb.
    QUrl serverUrl;
    serverUrl.setHost(QHostAddress(QHostAddress::LocalHost).toString());
    serverUrl.setPort(port.number());
    serverUrl.setScheme(urlTcpScheme());
    qCDebug(androidRunnerLog) << "Qml Server port ready"<< serverUrl;
    emit qmlServerReady(serverUrl);
}

void AndroidRunner::remoteOutput(const QString &output)
{
    appendMessage(output, Utils::StdOutFormat);
    m_outputParser.processOutput(output);
}

void AndroidRunner::remoteErrorOutput(const QString &output)
{
    appendMessage(output, Utils::StdErrFormat);
    m_outputParser.processOutput(output);
}

void AndroidRunner::handleRemoteProcessStarted(Utils::Port debugServerPort,
                                               const QUrl &qmlServer, qint64 pid)
{
    m_pid = ProcessHandle(pid);
    m_debugServerPort = debugServerPort;
    m_qmlServer = qmlServer;
    reportStarted();
}

void AndroidRunner::handleRemoteProcessFinished(const QString &errString)
{
    appendMessage(errString, Utils::NormalMessageFormat);
    if (runControl()->isRunning())
        runControl()->initiateStop();
    reportStopped();
}

void AndroidRunner::launchAVD()
{
    if (!m_target || !m_target->project())
        return;

    QStringList androidAbis = AndroidManager::applicationAbis(m_target);

    // Get AVD info
    const IDevice::ConstPtr device = DeviceKitAspect::device(m_target->kit());
    AndroidDeviceInfo info = AndroidDevice::androidDeviceInfoFromIDevice(device.data());
    AndroidManager::setDeviceSerialNumber(m_target, info.serialNumber);
    emit androidDeviceInfoChanged(info);
    if (info.isValid()) {
        AndroidAvdManager avdManager;
        if (!info.avdName.isEmpty() && avdManager.findAvd(info.avdName).isEmpty()) {
            bool launched = avdManager.startAvdAsync(info.avdName);
            m_launchedAVDName = launched ? info.avdName:"";
        } else {
            m_launchedAVDName.clear();
        }
    }
}

void AndroidRunner::checkAVD()
{
    const AndroidConfig &config = AndroidConfigurations::currentConfig();
    AndroidAvdManager avdManager(config);
    QString serialNumber = avdManager.findAvd(m_launchedAVDName);
    if (!serialNumber.isEmpty())
        return; // try again on next timer hit

    if (avdManager.isAvdBooted(serialNumber)) {
        m_checkAVDTimer.stop();
        AndroidManager::setDeviceSerialNumber(m_target, serialNumber);
        emit asyncStart();
    } else if (!config.isConnected(serialNumber)) {
        // device was disconnected
        m_checkAVDTimer.stop();
    }
}

} // namespace Internal
} // namespace Android
