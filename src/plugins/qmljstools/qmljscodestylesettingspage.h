// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/dialogs/ioptionspage.h>
#include <texteditor/icodestylepreferencesfactory.h>

#include <QWidget>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QSettings;
QT_END_NAMESPACE

namespace TextEditor {
    class FontSettings;
    class CodeStyleEditor;
    class SimpleCodeStylePreferencesWidget;
    class SnippetEditorWidget;
}

namespace QmlJSTools {
class QmlJSCodeStylePreferences;
class QmlJSCodeStylePreferencesWidget;
class QmlJSCodeStyleSettings;

namespace Internal {

class QmlJSCodeStylePreferencesWidget : public TextEditor::CodeStyleEditorWidget
{
    Q_OBJECT

public:
    explicit QmlJSCodeStylePreferencesWidget(const TextEditor::ICodeStylePreferencesFactory *factory,
                                             QWidget *parent = nullptr);

    void setPreferences(QmlJSCodeStylePreferences* preferences);

private:
    void decorateEditor(const TextEditor::FontSettings &fontSettings);
    void setVisualizeWhitespace(bool on);
    void slotSettingsChanged();
    void updatePreview();

    QmlJSCodeStylePreferences *m_preferences = nullptr;
    TextEditor::SimpleCodeStylePreferencesWidget *m_tabPreferencesWidget;
    QmlJSTools::QmlJSCodeStylePreferencesWidget *m_codeStylePreferencesWidget;
    TextEditor::SnippetEditorWidget *m_previewTextEdit;
};


class QmlJSCodeStyleSettingsPage : public Core::IOptionsPage
{
public:
    QmlJSCodeStyleSettingsPage();

    QWidget *widget() override;
    void apply() override;
    void finish() override;

private:
    QmlJSCodeStylePreferences *m_preferences = nullptr;
    QPointer<TextEditor::CodeStyleEditor> m_widget;
};

} // namespace Internal
} // namespace QmlJSTools
