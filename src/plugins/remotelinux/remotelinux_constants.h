// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

namespace RemoteLinux {
namespace Constants {

const char GenericLinuxOsType[] = "GenericLinuxOsType";

const char DeployToGenericLinux[] = "DeployToGenericLinux";

const char DirectUploadStepId[] = "RemoteLinux.DirectUploadStep";
const char MakeInstallStepId[] = "RemoteLinux.MakeInstall";
const char TarPackageCreationStepId[]  = "MaemoTarPackageCreationStep";
const char TarPackageFilePathId[] = "TarPackageFilePath";
const char TarPackageDeployStepId[] = "MaemoUploadAndInstallTarPackageStep";
const char RsyncDeployStepId[] = "RemoteLinux.RsyncDeployStep";
const char CustomCommandDeployStepId[] = "RemoteLinux.GenericRemoteLinuxCustomCommandDeploymentStep";
const char KillAppStepId[] = "RemoteLinux.KillAppStep";

const char SupportsRSync[] =  "RemoteLinux.SupportsRSync";

} // Constants
} // RemoteLinux
