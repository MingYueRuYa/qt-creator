// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <extensionsystem/iplugin.h>

namespace Ios {
namespace Internal {

class IosPlugin final : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Ios.json")

public:
    IosPlugin() = default;
    ~IosPlugin() final;

private:
    bool initialize(const QStringList &arguments, QString *errorMessage) final;

    class IosPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace Ios
