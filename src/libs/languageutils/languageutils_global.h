// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <qglobal.h>

#if defined(LANGUAGEUTILS_LIBRARY)
#  define LANGUAGEUTILS_EXPORT Q_DECL_EXPORT
#elif defined(LANGUAGEUTILS_STATIC_LIBRARY)
#  define LANGUAGEUTILS_EXPORT
#else
#  define LANGUAGEUTILS_EXPORT Q_DECL_IMPORT
#endif
