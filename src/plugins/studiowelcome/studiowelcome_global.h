// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

#if defined(STUDIOWELCOME_LIBRARY)
#  define STUDIOWELCOME_EXPORT Q_DECL_EXPORT
#else
#  define STUDIOWELCOME_EXPORT Q_DECL_IMPORT
#endif
