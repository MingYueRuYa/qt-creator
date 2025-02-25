// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "projectmacro.h"

#include <utils/algorithm.h>
#include <cctype>

namespace ProjectExplorer {

bool Macro::isValid() const
{
    return !key.isEmpty() && type != MacroType::Invalid;
}

QByteArray Macro::toByteArray() const
{
    switch (type) {
        case MacroType::Define: {
            if (value.isEmpty())
               return QByteArray("#define ") + key;
            return QByteArray("#define ") + key + ' ' + value;
        }
        case MacroType::Undefine: return QByteArray("#undef ") + key;
        case MacroType::Invalid: break;
    }

    return QByteArray();
}

QByteArray Macro::toByteArray(const Macros &macros)
{
    QByteArray text;

    for (const Macro &macro : macros) {
        const QByteArray macroText = macro.toByteArray();
        if (!macroText.isEmpty())
            text += macroText + '\n';
    }

    return  text;
}

Macros Macro::toMacros(const QByteArray &text)
{
    return tokensLinesToMacros(tokenizeLines(splitLines(text)));
}

Macro Macro::fromKeyValue(const QString &utf16text)
{
    return fromKeyValue(utf16text.toUtf8());
}

Macro Macro::fromKeyValue(const QByteArray &text)
{
    QByteArray key;
    QByteArray value;
    MacroType type = MacroType::Invalid;

    if (!text.isEmpty()) {
        type = MacroType::Define;

        int index = text.indexOf('=');

        if (index != -1) {
            key = text.left(index).trimmed();
            value = text.mid(index + 1).trimmed();
        } else {
            key = text.trimmed();
            value = "1";
        }
    }

    return Macro(key, value, type);
}

QByteArray Macro::toKeyValue(const QByteArray &prefix) const
{
    QByteArray keyValue;
    if (type != MacroType::Invalid)
        keyValue = prefix;

    if (value.isEmpty())
        keyValue += key + '=';
    else if (value == "1")
        keyValue += key;
    else
        keyValue += key + '=' + value;

    return keyValue;
}

static void removeCarriageReturn(QByteArray &line)
{
    if (line.endsWith('\r'))
        line.truncate(line.size() - 1);
}

static void removeCarriageReturns(QList<QByteArray> &lines)
{
    for (QByteArray &line : lines)
        removeCarriageReturn(line);
}

QList<QByteArray> Macro::splitLines(const QByteArray &text)
{
    QList<QByteArray> splitLines = text.split('\n');

    splitLines.removeAll("");
    removeCarriageReturns(splitLines);

    return splitLines;
}

QByteArray Macro::removeNonsemanticSpaces(QByteArray line)
{
    auto begin = line.begin();
    auto end = line.end();
    bool notInString = true;

    auto newEnd = std::unique(begin, end, [&] (char first, char second) {
        notInString = notInString && first != '\"';
        return notInString && (first == '#' || std::isspace(first)) && std::isspace(second);
    });

    line.truncate(line.size() - int(std::distance(newEnd, end)));

    return line.trimmed();
}

QList<QByteArray> Macro::tokenizeLine(const QByteArray &line)
{
    const QByteArray normalizedLine = removeNonsemanticSpaces(line);

    const auto begin = normalizedLine.begin();
    auto first = std::find(normalizedLine.begin(), normalizedLine.end(), ' ');
    const auto end = normalizedLine.end();

    QList<QByteArray> tokens;

    if (first != end) {
        auto second = std::find(std::next(first), normalizedLine.end(), ' ');
        tokens.append(QByteArray(begin, int(std::distance(begin, first))));

        std::advance(first, 1);
        tokens.append(QByteArray(first, int(std::distance(first, second))));

        if (second != end) {
            std::advance(second, 1);
            tokens.append(QByteArray(second, int(std::distance(second, end))));
        }
    }

   return tokens;
}

QList<QList<QByteArray>> Macro::tokenizeLines(const QList<QByteArray> &lines)
{
    QList<QList<QByteArray>> tokensLines = Utils::transform(lines, &Macro::tokenizeLine);

    return tokensLines;
}

Macro Macro::tokensToMacro(const QList<QByteArray> &tokens)
{
    Macro macro;

    if (tokens.size() >= 2 && tokens[0] == "#define") {
        macro.type = MacroType::Define;
        macro.key = tokens[1];

        if (tokens.size() >= 3)
            macro.value = tokens[2];
    }

    return macro;
}

Macros Macro::tokensLinesToMacros(const QList<QList<QByteArray>> &tokensLines)
{
    Macros macros;
    macros.reserve(tokensLines.size());

    for (const QList<QByteArray> &tokens : tokensLines) {
        Macro macro = tokensToMacro(tokens);

        if (macro.type != MacroType::Invalid)
            macros.push_back(std::move(macro));
    }

    return macros;
}

} // namespace ProjectExplorer
