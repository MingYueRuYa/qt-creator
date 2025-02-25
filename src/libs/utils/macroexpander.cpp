// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "macroexpander.h"

#include "algorithm.h"
#include "commandline.h"
#include "environment.h"
#include "qtcassert.h"
#include "stringutils.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMap>

namespace Utils {
namespace Internal {

static Q_LOGGING_CATEGORY(expanderLog, "qtc.utils.macroexpander", QtWarningMsg)

const char kFilePathPostfix[] = ":FilePath";
const char kPathPostfix[] = ":Path";
const char kNativeFilePathPostfix[] = ":NativeFilePath";
const char kNativePathPostfix[] = ":NativePath";
const char kFileNamePostfix[] = ":FileName";
const char kFileBaseNamePostfix[] = ":FileBaseName";

class MacroExpanderPrivate : public AbstractMacroExpander
{
public:
    MacroExpanderPrivate() = default;

    bool resolveMacro(const QString &name, QString *ret, QSet<AbstractMacroExpander *> &seen) override
    {
        // Prevent loops:
        const int count = seen.count();
        seen.insert(this);
        if (seen.count() == count)
            return false;

        bool found;
        *ret = value(name.toUtf8(), &found);
        if (found)
            return true;

        found = Utils::anyOf(m_subProviders, [name, ret, &seen] (const MacroExpanderProvider &p) -> bool {
            MacroExpander *expander = p ? p() : 0;
            return expander && expander->d->resolveMacro(name, ret, seen);
        });

        if (found)
            return true;

        found = Utils::anyOf(m_extraResolvers, [name, ret] (const MacroExpander::ResolverFunction &resolver) {
            return resolver(name, ret);
        });

        if (found)
            return true;

        return this == globalMacroExpander()->d ? false : globalMacroExpander()->d->resolveMacro(name, ret, seen);
    }

    QString value(const QByteArray &variable, bool *found) const
    {
        MacroExpander::StringFunction sf = m_map.value(variable);
        if (sf) {
            if (found)
                *found = true;
            return sf();
        }

        for (auto it = m_prefixMap.constBegin(); it != m_prefixMap.constEnd(); ++it) {
            if (variable.startsWith(it.key())) {
                MacroExpander::PrefixFunction pf = it.value();
                if (found)
                    *found = true;
                return pf(QString::fromUtf8(variable.mid(it.key().count())));
            }
        }
        if (found)
            *found = false;

        return QString();
    }

    QHash<QByteArray, MacroExpander::StringFunction> m_map;
    QHash<QByteArray, MacroExpander::PrefixFunction> m_prefixMap;
    QVector<MacroExpander::ResolverFunction> m_extraResolvers;
    QMap<QByteArray, QString> m_descriptions;
    QString m_displayName;
    QVector<MacroExpanderProvider> m_subProviders;
    bool m_accumulating = false;

    bool m_aborted = false;
    int m_lockDepth = 0;
};

} // Internal

using namespace Internal;

/*!
    \class Utils::MacroExpander
    \brief The MacroExpander class manages \QC wide variables, that a user
    can enter into many string settings. The variables are replaced by an actual value when the string
    is used, similar to how environment variables are expanded by a shell.

    \section1 Variables

    Variable names can be basically any string without dollar sign and braces,
    though it is recommended to only use 7-bit ASCII without special characters and whitespace.

    If there are several variables that contain different aspects of the same object,
    it is convention to give them the same prefix, followed by a colon and a postfix
    that describes the aspect.
    Examples of this are \c{CurrentDocument:FilePath} and \c{CurrentDocument:Selection}.

    When the variable manager is requested to replace variables in a string, it looks for
    variable names enclosed in %{ and }, like %{CurrentDocument:FilePath}.

    Environment variables are accessible using the %{Env:...} notation.
    For example, to access the SHELL environment variable, use %{Env:SHELL}.

    \note The names of the variables are stored as QByteArray. They are typically
    7-bit-clean. In cases where this is not possible, UTF-8 encoding is
    assumed.

    \section1 Providing Variable Values

    Plugins can register variables together with a description through registerVariable().
    A typical setup is to register variables in the Plugin::initialize() function.

    \code
    bool MyPlugin::initialize(const QStringList &arguments, QString *errorString)
    {
        [...]
        MacroExpander::registerVariable(
            "MyVariable",
            tr("The current value of whatever I want."));
            []() -> QString {
                QString value;
                // do whatever is necessary to retrieve the value
                [...]
                return value;
            }
        );
        [...]
    }
    \endcode


    For variables that refer to a file, you should use the convenience function
    MacroExpander::registerFileVariables().
    The functions take a variable prefix, like \c MyFileVariable,
    and automatically handle standardized postfixes like \c{:FilePath},
    \c{:Path} and \c{:FileBaseName}, resulting in the combined variables, such as
    \c{MyFileVariable:FilePath}.

    \section1 Providing and Expanding Parametrized Strings

    Though it is possible to just ask the variable manager for the value of some variable in your
    code, the preferred use case is to give the user the possibility to parametrize strings, for
    example for settings.

    (If you ever think about doing the former, think twice. It is much more efficient
    to just ask the plugin that provides the variable value directly, without going through
    string conversions, and through the variable manager which will do a large scale poll. To be
    more concrete, using the example from the Providing Variable Values section: instead of
    calling \c{MacroExpander::value("MyVariable")}, it is much more efficient to just ask directly
    with \c{MyPlugin::variableValue()}.)

    \section2 User Interface

    If the string that you want to parametrize is settable by the user, through a QLineEdit or
    QTextEdit derived class, you should add a variable chooser to your UI, which allows adding
    variables to the string by browsing through a list. See Utils::VariableChooser for more
    details.

    \section2 Expanding Strings

    Expanding variable values in strings is done by "macro expanders".
    Utils::AbstractMacroExpander is the base class for these, and the variable manager
    provides an implementation that expands \QC variables through
    MacroExpander::macroExpander().

    There are several different ways to expand a string, covering the different use cases,
    listed here sorted by relevance:
    \list
    \li Using MacroExpander::expandedString(). This is the most comfortable way to get a string
        with variable values expanded, but also the least flexible one. If this is sufficient for
        you, use it.
    \li Using the Utils::expandMacros() functions. These take a string and a macro expander (for which
        you would use the one provided by the variable manager). Mostly the same as
        MacroExpander::expandedString(), but also has a variant that does the replacement inline
        instead of returning a new string.
    \li Using Utils::QtcProcess::expandMacros(). This expands the string while conforming to the
        quoting rules of the platform it is run on. Use this function with the variable manager's
        macro expander if your string will be passed as a command line parameter string to an
        external command.
    \li Writing your own macro expander that nests the variable manager's macro expander. And then
        doing one of the above. This allows you to expand additional "local" variables/macros,
        that do not come from the variable manager.
    \endlist

*/

/*!
 * \internal
 */
MacroExpander::MacroExpander()
{
    d = new MacroExpanderPrivate;
}

/*!
 * \internal
 */
MacroExpander::~MacroExpander()
{
    delete d;
}

/*!
 * \internal
 */
bool MacroExpander::resolveMacro(const QString &name, QString *ret) const
{
    QSet<AbstractMacroExpander*> seen;
    return d->resolveMacro(name, ret, seen);
}

/*!
 * Returns the value of the given \a variable. If \a found is given, it is
 * set to true if the variable has a value at all, false if not.
 */
QString MacroExpander::value(const QByteArray &variable, bool *found) const
{
    return d->value(variable, found);
}

/*!
 * Returns \a stringWithVariables with all variables replaced by their values.
 * See the MacroExpander overview documentation for other ways to expand variables.
 *
 * \sa MacroExpander
 * \sa macroExpander()
 */
QString MacroExpander::expand(const QString &stringWithVariables) const
{
    if (d->m_lockDepth == 0)
        d->m_aborted = false;

    if (d->m_lockDepth > 10) { // Limit recursion.
        d->m_aborted = true;
        return QString();
    }

    ++d->m_lockDepth;

    QString res = stringWithVariables;
    Utils::expandMacros(&res, d);

    --d->m_lockDepth;

    if (d->m_lockDepth == 0 && d->m_aborted)
        return tr("Infinite recursion error") + QLatin1String(": ") + stringWithVariables;

    return res;
}

FilePath MacroExpander::expand(const FilePath &fileNameWithVariables) const
{
    // We want single variables to expand to fully qualified strings.
    return FilePath::fromString(expand(fileNameWithVariables.toString()));
}

QByteArray MacroExpander::expand(const QByteArray &stringWithVariables) const
{
    return expand(QString::fromLatin1(stringWithVariables)).toLatin1();
}

QVariant MacroExpander::expandVariant(const QVariant &v) const
{
    const auto type = QMetaType::Type(v.type());
    if (type == QMetaType::QString) {
        return expand(v.toString());
    } else if (type == QMetaType::QStringList) {
        return Utils::transform(v.toStringList(),
                                [this](const QString &s) -> QVariant { return expand(s); });
    } else if (type == QMetaType::QVariantList) {
        return Utils::transform(v.toList(), [this](const QVariant &v) { return expandVariant(v); });
    } else if (type == QMetaType::QVariantMap) {
        const auto map = v.toMap();
        QVariantMap result;
        for (auto it = map.cbegin(), end = map.cend(); it != end; ++it)
            result.insert(it.key(), expandVariant(it.value()));
        return result;
    }
    return v;
}

QString MacroExpander::expandProcessArgs(const QString &argsWithVariables) const
{
    QString result = argsWithVariables;
    const bool ok = ProcessArgs::expandMacros(&result, d);
    QTC_ASSERT(ok, qCDebug(expanderLog) << "Expanding failed: " << argsWithVariables);
    return result;
}

static QByteArray fullPrefix(const QByteArray &prefix)
{
    QByteArray result = prefix;
    if (!result.endsWith(':'))
        result.append(':');
    return result;
}

/*!
 * Makes the given string-valued \a prefix known to the variable manager,
 * together with a localized \a description.
 *
 * The \a value PrefixFunction will be called and gets the full variable name
 * with the prefix stripped as input.
 *
 * \sa registerVariables(), registerIntVariable(), registerFileVariables()
 */
void MacroExpander::registerPrefix(const QByteArray &prefix, const QString &description,
                                   const MacroExpander::PrefixFunction &value, bool visible)
{
    QByteArray tmp = fullPrefix(prefix);
    if (visible)
        d->m_descriptions.insert(tmp + "<value>", description);
    d->m_prefixMap.insert(tmp, value);
}

/*!
 * Makes the given string-valued \a variable known to the variable manager,
 * together with a localized \a description.
 *
 * \sa registerFileVariables(), registerIntVariable(), registerPrefix()
 */
void MacroExpander::registerVariable(const QByteArray &variable,
    const QString &description, const StringFunction &value, bool visibleInChooser)
{
    if (visibleInChooser)
        d->m_descriptions.insert(variable, description);
    d->m_map.insert(variable, value);
}

/*!
 * Makes the given integral-valued \a variable known to the variable manager,
 * together with a localized \a description.
 *
 * \sa registerVariable(), registerFileVariables(), registerPrefix()
 */
void MacroExpander::registerIntVariable(const QByteArray &variable,
    const QString &description, const MacroExpander::IntFunction &value)
{
    const MacroExpander::IntFunction valuecopy = value; // do not capture a reference in a lambda
    registerVariable(variable, description,
        [valuecopy]() { return QString::number(valuecopy ? valuecopy() : 0); });
}

/*!
 * Convenience function to register several variables with the same \a prefix, that have a file
 * as a value. Takes the prefix and registers variables like \c{prefix:FilePath} and
 * \c{prefix:Path}, with descriptions that start with the given \a heading.
 * For example \c{registerFileVariables("CurrentDocument", tr("Current Document"))} registers
 * variables such as \c{CurrentDocument:FilePath} with description
 * "Current Document: Full path including file name."
 *
 * \sa registerVariable(), registerIntVariable(), registerPrefix()
 */
void MacroExpander::registerFileVariables(const QByteArray &prefix,
    const QString &heading, const FileFunction &base, bool visibleInChooser)
{
    registerVariable(prefix + kFilePathPostfix,
         tr("%1: Full path including file name.").arg(heading),
         [base]() -> QString { QString tmp = base().toString(); return tmp.isEmpty() ? QString() : QFileInfo(tmp).filePath(); },
         visibleInChooser);

    registerVariable(prefix + kPathPostfix,
         tr("%1: Full path excluding file name.").arg(heading),
         [base]() -> QString { QString tmp = base().toString(); return tmp.isEmpty() ? QString() : QFileInfo(tmp).path(); },
         visibleInChooser);

    registerVariable(prefix + kNativeFilePathPostfix,
         tr("%1: Full path including file name, with native path separator (backslash on Windows).").arg(heading),
         [base]() -> QString {
             QString tmp = base().toString();
             return tmp.isEmpty() ? QString() : QDir::toNativeSeparators(QFileInfo(tmp).filePath());
         },
         visibleInChooser);

    registerVariable(prefix + kNativePathPostfix,
         tr("%1: Full path excluding file name, with native path separator (backslash on Windows).").arg(heading),
         [base]() -> QString {
             QString tmp = base().toString();
             return tmp.isEmpty() ? QString() : QDir::toNativeSeparators(QFileInfo(tmp).path());
         },
         visibleInChooser);

    registerVariable(prefix + kFileNamePostfix,
         tr("%1: File name without path.").arg(heading),
         [base]() -> QString { QString tmp = base().toString(); return tmp.isEmpty() ? QString() : FilePath::fromString(tmp).fileName(); },
         visibleInChooser);

    registerVariable(prefix + kFileBaseNamePostfix,
         tr("%1: File base name without path and suffix.").arg(heading),
         [base]() -> QString { QString tmp = base().toString(); return tmp.isEmpty() ? QString() : QFileInfo(tmp).baseName(); },
         visibleInChooser);
}

void MacroExpander::registerExtraResolver(const MacroExpander::ResolverFunction &value)
{
    d->m_extraResolvers.append(value);
}

/*!
 * Returns all registered variable names.
 *
 * \sa registerVariable()
 * \sa registerFileVariables()
 */
QList<QByteArray> MacroExpander::visibleVariables() const
{
    return d->m_descriptions.keys();
}

/*!
 * Returns the description that was registered for the \a variable.
 */
QString MacroExpander::variableDescription(const QByteArray &variable) const
{
    return d->m_descriptions.value(variable);
}

bool MacroExpander::isPrefixVariable(const QByteArray &variable) const
{
    return d->m_prefixMap.contains(fullPrefix(variable));
}

MacroExpanderProviders MacroExpander::subProviders() const
{
    return d->m_subProviders;
}

QString MacroExpander::displayName() const
{
    return d->m_displayName;
}

void MacroExpander::setDisplayName(const QString &displayName)
{
    d->m_displayName = displayName;
}

void MacroExpander::registerSubProvider(const MacroExpanderProvider &provider)
{
    d->m_subProviders.append(provider);
}

bool MacroExpander::isAccumulating() const
{
    return d->m_accumulating;
}

void MacroExpander::setAccumulating(bool on)
{
    d->m_accumulating = on;
}

class GlobalMacroExpander : public MacroExpander
{
public:
    GlobalMacroExpander()
    {
        setDisplayName(MacroExpander::tr("Global variables"));
        registerPrefix("Env",
                       MacroExpander::tr("Access environment variables."),
                       [](const QString &value) { return qtcEnvironmentVariable(value); });
    }
};

/*!
 * Returns the expander for globally registered variables.
 */
MacroExpander *globalMacroExpander()
{
    static GlobalMacroExpander theGlobalExpander;
    return &theGlobalExpander;
}

} // namespace Utils
