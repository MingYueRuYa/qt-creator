// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/buildstep.h>

namespace Ios {
namespace Internal {

class IosDeployStepFactory final : public ProjectExplorer::BuildStepFactory
{
public:
    IosDeployStepFactory();
};

} // namespace Internal
} // namespace Ios
