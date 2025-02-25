# Copyright (C) 2016 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

try:
    import __builtin__                  # Python 2
except ImportError:
    import builtins as __builtin__      # Python 3

# for easier re-usage (because Python hasn't an enum type)
class Targets:
    ALL_TARGETS = tuple(range(3))

    (DESKTOP_5_4_1_GCC,
     DESKTOP_5_10_1_DEFAULT,
     DESKTOP_5_14_1_DEFAULT) = ALL_TARGETS

    __TARGET_NAME_DICT__ = dict(zip(ALL_TARGETS,
                                    ["Desktop 5.4.1 GCC",
                                     "Desktop 5.10.1 default",
                                     "Desktop 5.14.1 default"]))

    @staticmethod
    def availableTargetClasses(ignoreValidity=False):
        availableTargets = set(Targets.ALL_TARGETS)
        if platform.system() == 'Darwin':
            availableTargets.remove(Targets.DESKTOP_5_4_1_GCC)
        return availableTargets

    @staticmethod
    def desktopTargetClasses():
        return Targets.availableTargetClasses()

    @staticmethod
    def getStringForTarget(target):
        return Targets.__TARGET_NAME_DICT__[target]

    @staticmethod
    def getTargetsAsStrings(targets):
        return set(map(Targets.getStringForTarget, targets))

    @staticmethod
    def getIdForTargetName(targetName):
        return {v:k for k, v in Targets.__TARGET_NAME_DICT__.items()}[targetName]

    @staticmethod
    def getDefaultKit():
        return Targets.DESKTOP_5_14_1_DEFAULT

# this class holds some constants for easier usage inside the Projects view
class ProjectSettings:
    BUILD = 1
    RUN = 2

# this class defines some constants for the views of the creator's MainWindow
class ViewConstants:
    WELCOME, EDIT, DESIGN, DEBUG, PROJECTS, HELP = range(6)
    FIRST_AVAILABLE = 0
    # always adjust the following to the highest value of the available ViewConstants when adding new
    LAST_AVAILABLE = HELP

class LibType:
    SHARED = 0
    STATIC = 1
    QT_PLUGIN = 2

    @staticmethod
    def getStringForLib(libType):
        if libType == LibType.SHARED:
            return "Shared Library"
        if libType == LibType.STATIC:
            return "Statically Linked Library"
        if libType == LibType.QT_PLUGIN:
            return "Qt Plugin"
        return None

class Qt5Path:
    DOCS = 0
    EXAMPLES = 1

    @staticmethod
    def getPaths(pathSpec):
        qt5targets = [Targets.DESKTOP_5_10_1_DEFAULT, Targets.DESKTOP_5_14_1_DEFAULT]
        if platform.system() != 'Darwin':
            qt5targets.append(Targets.DESKTOP_5_4_1_GCC)
        if pathSpec == Qt5Path.DOCS:
            return map(lambda target: Qt5Path.docsPath(target), qt5targets)
        elif pathSpec == Qt5Path.EXAMPLES:
            return map(lambda target: Qt5Path.examplesPath(target), qt5targets)
        else:
            test.fatal("Unknown pathSpec given: %s" % str(pathSpec))
            return []

    @staticmethod
    def __preCheckAndExtractQtVersionStr__(target):
        if target not in Targets.ALL_TARGETS:
            raise Exception("Unexpected target '%s'" % str(target))

        matcher = re.match("^Desktop (5\.\\d{1,2}\.\\d{1,2}).*$", Targets.getStringForTarget(target))
        if matcher is None:
            raise Exception("Currently this is supported for Desktop Qt5 only, got target '%s'"
                            % str(Targets.getStringForTarget(target)))
        return matcher.group(1)

    @staticmethod
    def __createPlatformQtPath__(qt5Minor):
        if platform.system() in ('Microsoft', 'Windows'):
            return "C:/Qt/Qt5.%d.1" % qt5Minor
        else:
            return os.path.expanduser("~/Qt5.%d.1" % qt5Minor)

    @staticmethod
    def toVersionTuple(versionString):
        return tuple(map(__builtin__.int, versionString.split(".")))

    @staticmethod
    def getQtMinorAndPatchVersion(target):
        qtVersionStr = Qt5Path.__preCheckAndExtractQtVersionStr__(target)
        versionTuple = Qt5Path.toVersionTuple(qtVersionStr)
        return versionTuple[1], versionTuple[2]

    @staticmethod
    def examplesPath(target):
        qtMinorVersion, qtPatchVersion = Qt5Path.getQtMinorAndPatchVersion(target)
        if qtMinorVersion < 10:
            path = "Examples/Qt-5.%d" % qtMinorVersion
        else:
            path = "Examples/Qt-5.%d.%d" % (qtMinorVersion, qtPatchVersion)

        return os.path.join(Qt5Path.__createPlatformQtPath__(qtMinorVersion), path)

    @staticmethod
    def docsPath(target):
        qtMinorVersion, qtPatchVersion = Qt5Path.getQtMinorAndPatchVersion(target)
        if qtMinorVersion < 10:
            path = "Docs/Qt-5.%d" % qtMinorVersion
        else:
            path = "Docs/Qt-5.%d.%d" % (qtMinorVersion, qtPatchVersion)

        return os.path.join(Qt5Path.__createPlatformQtPath__(qtMinorVersion), path)

class TestSection:
    def __init__(self, description):
        self.description = description

    def __enter__(self):
        test.startSection(self.description)

    def __exit__(self, exc_type, exc_value, traceback):
        test.endSection()
