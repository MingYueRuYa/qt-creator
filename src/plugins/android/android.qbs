import qbs 1.0

Project {
    name: "Android"

    QtcPlugin {
        Depends { name: "Qt"; submodules: ["widgets", "xml", "network"] }
        Depends { name: "Core" }
        Depends { name: "Debugger" }
        Depends { name: "LanguageClient" }
        Depends { name: "LanguageServerProtocol" }
        Depends { name: "ProParser" }
        Depends { name: "ProjectExplorer" }
        Depends { name: "QmlDebug" }
        Depends { name: "QtSupport" }
        Depends { name: "TextEditor" }
        Depends { name: "Utils" }
        Depends { name: "app_version_header" }

        files: [
            "android_global.h", "androidtr.h",
            "android.qrc",
            "androidavdmanager.cpp",
            "androidavdmanager.h",
            "androidconfigurations.cpp",
            "androidconfigurations.h",
            "androidconstants.h",
            "androidcreatekeystorecertificate.cpp",
            "androidcreatekeystorecertificate.h",
            "androidbuildapkstep.cpp",
            "androidbuildapkstep.h",
            "androiddeployqtstep.cpp",
            "androiddeployqtstep.h",
            "androiddebugsupport.cpp",
            "androiddebugsupport.h",
            "androiddevice.cpp",
            "androiddevice.h",
            "androiddeviceinfo.cpp",
            "androiddeviceinfo.h",
            "androiderrormessage.h",
            "androiderrormessage.cpp",
            "androidextralibrarylistmodel.cpp",
            "androidextralibrarylistmodel.h",
            "androidglobal.h",
            "androidmanager.cpp",
            "androidmanager.h",
            "androidmanifestdocument.cpp",
            "androidmanifestdocument.h",
            "androidmanifesteditor.cpp",
            "androidmanifesteditor.h",
            "androidmanifesteditoriconwidget.cpp",
            "androidmanifesteditoriconwidget.h",
            "androidmanifesteditoriconcontainerwidget.cpp",
            "androidmanifesteditoriconcontainerwidget.h",
            "androidmanifesteditorfactory.cpp",
            "androidmanifesteditorfactory.h",
            "androidmanifesteditorwidget.cpp",
            "androidmanifesteditorwidget.h",
            "androidpackageinstallationstep.cpp",
            "androidpackageinstallationstep.h",
            "androidplugin.cpp",
            "androidplugin.h",
            "androidpotentialkit.cpp",
            "androidpotentialkit.h",
            "androidqmlpreviewworker.h",
            "androidqmlpreviewworker.cpp",
            "androidqmltoolingsupport.cpp",
            "androidqmltoolingsupport.h",
            "androidqtversion.cpp",
            "androidqtversion.h",
            "androidrunconfiguration.cpp",
            "androidrunconfiguration.h",
            "androidruncontrol.cpp",
            "androidruncontrol.h",
            "androidrunner.cpp",
            "androidrunner.h",
            "androidrunnerworker.cpp",
            "androidrunnerworker.h",
            "androidsdkdownloader.cpp",
            "androidsdkdownloader.h",
            "androidsdkmanager.cpp",
            "androidsdkmanager.h",
            "androidsdkmanagerwidget.cpp",
            "androidsdkmanagerwidget.h",
            "androidsdkmodel.cpp",
            "androidsdkmodel.h",
            "androidsdkpackage.cpp",
            "androidsdkpackage.h",
            "androidservicewidget.cpp",
            "androidservicewidget.h",
            "androidservicewidget_p.h",
            "androidsettingswidget.cpp",
            "androidsettingswidget.h",
            "androidsignaloperation.cpp",
            "androidsignaloperation.h",
            "androidtoolchain.cpp",
            "androidtoolchain.h",
            "avddialog.cpp",
            "avddialog.h",
            "avdmanageroutputparser.cpp",
            "avdmanageroutputparser.h",
            "certificatesmodel.cpp",
            "certificatesmodel.h",
            "createandroidmanifestwizard.h",
            "createandroidmanifestwizard.cpp",
            "javaeditor.cpp",
            "javaeditor.h",
            "javaindenter.cpp",
            "javaindenter.h",
            "javalanguageserver.cpp",
            "javalanguageserver.h",
            "javaparser.cpp",
            "javaparser.h",
            "splashscreencontainerwidget.cpp",
            "splashscreencontainerwidget.h",
            "splashscreenwidget.cpp",
            "splashscreenwidget.h"
        ]

        Group {
            name: "Unit tests"
            condition: qtc.testsEnabled
            files: [
                "android_tst.qrc",
            ]
        }
    }
}
