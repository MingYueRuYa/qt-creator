// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "customwidget.h"

#include <utils/newclasswidget.h>
#include <utils/classnamevalidatinglineedit.h>
#include <utils/filenamevalidatinglineedit.h>
#include <utils/linecolumnlabel.h>
#include <utils/pathchooser.h>
#include <utils/projectnamevalidatinglineedit.h>
#include <utils/fancylineedit.h>
#include <utils/qtcolorbutton.h>
#include <utils/submiteditorwidget.h>
#include <utils/submitfieldwidget.h>
#include <utils/pathlisteditor.h>
#include <utils/detailsbutton.h>
#include <utils/detailswidget.h>
#include <utils/styledbar.h>
#include <utils/wizard.h>
#include <utils/crumblepath.h>

#include <QDesignerCustomWidgetCollectionInterface>
#include <QDesignerContainerExtension>
#include <QExtensionFactory>

#include <qplugin.h>
#include <QList>

QT_BEGIN_NAMESPACE
class QExtensionManager;
QT_END_NAMESPACE

// Custom Widgets

class NewClassCustomWidget :
    public QObject,
    public CustomWidget<Utils::NewClassWidget>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit NewClassCustomWidget(QObject *parent = 0);
};

class ClassNameValidatingLineEdit_CW :
    public QObject,
    public CustomWidget<Utils::ClassNameValidatingLineEdit>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit ClassNameValidatingLineEdit_CW(QObject *parent = 0);
};

class FileNameValidatingLineEdit_CW :
    public QObject,
    public CustomWidget<Utils::FileNameValidatingLineEdit>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit FileNameValidatingLineEdit_CW(QObject *parent = 0);
};

class ProjectNameValidatingLineEdit_CW :
    public QObject,
    public CustomWidget<Utils::ProjectNameValidatingLineEdit>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit ProjectNameValidatingLineEdit_CW(QObject *parent = 0);
};

class LineColumnLabel_CW :
    public QObject,
    public CustomWidget<Utils::LineColumnLabel>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit LineColumnLabel_CW(QObject *parent = 0);
};

class PathChooser_CW :
    public QObject,
    public CustomWidget<Utils::PathChooser>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit PathChooser_CW(QObject *parent = 0);
};

class IconButton_CW :
    public QObject,
    public CustomWidget<Utils::IconButton>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit IconButton_CW(QObject *parent = 0);
};

class FancyLineEdit_CW :
    public QObject,
    public CustomWidget<Utils::FancyLineEdit>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit FancyLineEdit_CW(QObject *parent = 0);

    virtual QWidget *createWidget(QWidget *parent);
};

class QtColorButton_CW :
    public QObject,
    public CustomWidget<Utils::QtColorButton>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit QtColorButton_CW(QObject *parent = 0);
};

class SubmitEditorWidget_CW :
    public QObject,
    public CustomWidget<Utils::SubmitEditorWidget>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit SubmitEditorWidget_CW(QObject *parent = 0);
};

class SubmitFieldWidget_CW :
    public QObject,
    public CustomWidget<Utils::SubmitFieldWidget>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit SubmitFieldWidget_CW(QObject *parent = 0);
};

class PathListEditor_CW :
    public QObject,
    public CustomWidget<Utils::PathListEditor>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit PathListEditor_CW(QObject *parent = 0);
};

class DetailsButton_CW :
    public QObject,
    public CustomWidget<Utils::DetailsButton>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit DetailsButton_CW(QObject *parent = 0);
};

class StyledBar_CW :
    public QObject,
    public CustomWidget<Utils::StyledBar>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit StyledBar_CW(QObject *parent = 0);
};

class StyledSeparator_CW :
    public QObject,
    public CustomWidget<Utils::StyledSeparator>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit StyledSeparator_CW(QObject *parent = 0);
};

class Wizard_CW :
    public QObject,
    public CustomWidget<Utils::Wizard>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit Wizard_CW(QObject *parent = 0);
};

class CrumblePath_CW :
    public QObject,
    public CustomWidget<Utils::CrumblePath>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit CrumblePath_CW(QObject *parent = 0);
};

// Details Widget: plugin + simple, hacky container extension that
// accepts only one page.

class DetailsWidget_CW :
    public QObject,
    public CustomWidget<Utils::DetailsWidget>
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
    explicit DetailsWidget_CW(QObject *parent = 0);
    QString domXml() const;
    void initialize(QDesignerFormEditorInterface *core);
};

class DetailsWidgetContainerExtension: public QObject,
                                         public QDesignerContainerExtension
{
    Q_OBJECT
    Q_INTERFACES(QDesignerContainerExtension)
public:
    explicit DetailsWidgetContainerExtension(Utils::DetailsWidget *widget, QObject *parent);

    void addWidget(QWidget *widget);
    int count() const;
    int currentIndex() const;
    void insertWidget(int index, QWidget *widget);
    void remove(int index);
    void setCurrentIndex(int index);
    QWidget *widget(int index) const;

private:
    Utils::DetailsWidget *m_detailsWidget;
};

class DetailsWidgetExtensionFactory: public QExtensionFactory
{
    Q_OBJECT
public:
    explicit DetailsWidgetExtensionFactory(QExtensionManager *parent = 0);

protected:
    QObject *createExtension(QObject *object, const QString &iid, QObject *parent) const;
};

// ------------ Collection

class WidgetCollection : public QObject, public QDesignerCustomWidgetCollectionInterface
{
    Q_OBJECT
    Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)
public:
    explicit WidgetCollection(QObject *parent = 0);

    virtual QList<QDesignerCustomWidgetInterface*> customWidgets() const;

private:
    QList<QDesignerCustomWidgetInterface*> m_plugins;
};
