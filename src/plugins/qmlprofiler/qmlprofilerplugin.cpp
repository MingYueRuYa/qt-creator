// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qmlprofilerplugin.h"
#include "qmlprofilerrunconfigurationaspect.h"
#include "qmlprofilerruncontrol.h"
#include "qmlprofilersettings.h"
#include "qmlprofilertool.h"
#include "qmlprofileractions.h"

#ifdef WITH_TESTS

#include "tests/debugmessagesmodel_test.h"
#include "tests/flamegraphmodel_test.h"
#include "tests/flamegraphview_test.h"
#include "tests/inputeventsmodel_test.h"
#include "tests/localqmlprofilerrunner_test.h"
#include "tests/memoryusagemodel_test.h"
#include "tests/pixmapcachemodel_test.h"
#include "tests/qmlevent_test.h"
#include "tests/qmleventlocation_test.h"
#include "tests/qmleventtype_test.h"
#include "tests/qmlnote_test.h"
#include "tests/qmlprofileranimationsmodel_test.h"
#include "tests/qmlprofilerattachdialog_test.h"
#include "tests/qmlprofilerbindingloopsrenderpass_test.h"
#include "tests/qmlprofilerclientmanager_test.h"
#include "tests/qmlprofilerdetailsrewriter_test.h"
#include "tests/qmlprofilertool_test.h"
#include "tests/qmlprofilertraceclient_test.h"
#include "tests/qmlprofilertraceview_test.h"

// Force QML Debugging to be enabled, so that we can selftest the profiler
#define QT_QML_DEBUG_NO_WARNING
#include <QQmlDebuggingEnabler>
#include <QQmlEngine>
#undef QT_QML_DEBUG_NO_WARNING

#endif // WITH_TESTS

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/environmentaspect.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/runconfiguration.h>
#include <projectexplorer/target.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;

namespace QmlProfiler {
namespace Internal {

Q_GLOBAL_STATIC(QmlProfilerSettings, qmlProfilerGlobalSettings)

class QmlProfilerPluginPrivate
{
public:
    QmlProfilerTool m_profilerTool;
    QmlProfilerOptionsPage m_profilerOptionsPage;
    QmlProfilerActions m_actions;

    // The full local profiler.
    RunWorkerFactory localQmlProfilerFactory {
        RunWorkerFactory::make<LocalQmlProfilerSupport>(),
        {ProjectExplorer::Constants::QML_PROFILER_RUN_MODE},
        {},
        {ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE}
    };

    // The bits plugged in in remote setups.
    RunWorkerFactory qmlProfilerWorkerFactory {
        RunWorkerFactory::make<QmlProfilerRunner>(),
        {ProjectExplorer::Constants::QML_PROFILER_RUNNER},
        {},
        {}
    };
};

bool QmlProfilerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    return Utils::HostOsInfo::canCreateOpenGLContext(errorString);
}

void QmlProfilerPlugin::extensionsInitialized()
{
    d = new QmlProfilerPluginPrivate;
    d->m_actions.attachToTool(&d->m_profilerTool);
    d->m_actions.registerActions();

    RunConfiguration::registerAspect<QmlProfilerRunConfigurationAspect>();
}

ExtensionSystem::IPlugin::ShutdownFlag QmlProfilerPlugin::aboutToShutdown()
{
    delete d;
    d = nullptr;

    // Save settings.
    // Disconnect from signals that are not needed during shutdown
    // Hide UI (if you add UI that is not in the main window directly)
    return SynchronousShutdown;
}

QmlProfilerSettings *QmlProfilerPlugin::globalSettings()
{
    return qmlProfilerGlobalSettings();
}

QVector<QObject *> QmlProfiler::Internal::QmlProfilerPlugin::createTestObjects() const
{
    QVector<QObject *> tests;
#ifdef WITH_TESTS
    tests << new DebugMessagesModelTest;
    tests << new FlameGraphModelTest;
    tests << new FlameGraphViewTest;
    tests << new InputEventsModelTest;
    tests << new LocalQmlProfilerRunnerTest;
    tests << new MemoryUsageModelTest;
    tests << new PixmapCacheModelTest;
    tests << new QmlEventTest;
    tests << new QmlEventLocationTest;
    tests << new QmlEventTypeTest;
    tests << new QmlNoteTest;
    tests << new QmlProfilerAnimationsModelTest;
    tests << new QmlProfilerAttachDialogTest;
    tests << new QmlProfilerBindingLoopsRenderPassTest;
    tests << new QmlProfilerClientManagerTest;
    tests << new QmlProfilerDetailsRewriterTest;
    tests << new QmlProfilerToolTest;
    tests << new QmlProfilerTraceClientTest;
    tests << new QmlProfilerTraceViewTest;

    tests << new QQmlEngine; // Trigger debug connector to be started
#endif
    return tests;
}

} // namespace Internal
} // namespace QmlProfiler
