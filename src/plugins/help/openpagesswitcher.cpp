// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "openpagesswitcher.h"

#include "openpageswidget.h"

#include <utils/hostosinfo.h>

#include <QEvent>

#include <QKeyEvent>
#include <QVBoxLayout>

using namespace Help::Internal;

const int gWidth = 300;
const int gHeight = 200;

OpenPagesSwitcher::OpenPagesSwitcher(QAbstractItemModel *model)
    : QFrame(nullptr, Qt::Popup)
    , m_openPagesModel(model)
{
    resize(gWidth, gHeight);

    m_openPagesWidget = new OpenPagesWidget(m_openPagesModel, this);

    // We disable the frame on this list view and use a QFrame around it instead.
    // This improves the look with QGTKStyle.
    if (!Utils::HostOsInfo::isMacHost())
        setFrameStyle(m_openPagesWidget->frameStyle());
    m_openPagesWidget->setFrameStyle(QFrame::NoFrame);

    m_openPagesWidget->allowContextMenu(false);
    m_openPagesWidget->installEventFilter(this);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_openPagesWidget);

    connect(m_openPagesWidget, &OpenPagesWidget::closePage,
            this, &OpenPagesSwitcher::closePage);
    connect(m_openPagesWidget, &OpenPagesWidget::setCurrentPage,
            this, &OpenPagesSwitcher::setCurrentPage);
}

OpenPagesSwitcher::~OpenPagesSwitcher() = default;

void OpenPagesSwitcher::gotoNextPage()
{
    selectPageUpDown(-1);
}

void OpenPagesSwitcher::gotoPreviousPage()
{
    selectPageUpDown(1);
}

void OpenPagesSwitcher::selectAndHide()
{
    setVisible(false);
    QModelIndex index = m_openPagesWidget->currentIndex();
    if (index.isValid())
        emit setCurrentPage(index);
}

void OpenPagesSwitcher::selectCurrentPage(int index)
{
    m_openPagesWidget->selectCurrentPage(index);
}

void OpenPagesSwitcher::setVisible(bool visible)
{
    QWidget::setVisible(visible);
    if (visible)
        setFocus();
}

void OpenPagesSwitcher::focusInEvent(QFocusEvent *event)
{
    Q_UNUSED(event)
    m_openPagesWidget->setFocus();
}

bool OpenPagesSwitcher::eventFilter(QObject *object, QEvent *event)
{
    if (object == m_openPagesWidget) {
        if (event->type() == QEvent::KeyPress) {
            auto ke = static_cast<const QKeyEvent*>(event);
            if (ke->key() == Qt::Key_Escape) {
                setVisible(false);
                return true;
            }

            const int key = ke->key();
            if (key == Qt::Key_Return || key == Qt::Key_Enter || key == Qt::Key_Space) {
                emit setCurrentPage(m_openPagesWidget->currentIndex());
                return true;
            }
            const Qt::KeyboardModifiers modifier = Utils::HostOsInfo::isMacHost()
                    ? Qt::AltModifier : Qt::ControlModifier;
            if (key == Qt::Key_Backtab
                && (ke->modifiers() == (modifier | Qt::ShiftModifier)))
                gotoNextPage();
            else if (key == Qt::Key_Tab && (ke->modifiers() == modifier))
                gotoPreviousPage();
        } else if (event->type() == QEvent::KeyRelease) {
            auto ke = static_cast<const QKeyEvent*>(event);
            if (ke->modifiers() == 0
               /*HACK this is to overcome some event inconsistencies between platforms*/
               || (ke->modifiers() == Qt::AltModifier
               && (ke->key() == Qt::Key_Alt || ke->key() == -1))) {
                selectAndHide();
            }
        }
    }
    return QWidget::eventFilter(object, event);
}

void OpenPagesSwitcher::selectPageUpDown(int summand)
{
    const int pageCount = m_openPagesModel->rowCount();
    if (pageCount < 2)
        return;

    const QModelIndexList &list = m_openPagesWidget->selectionModel()->selectedIndexes();
    if (list.isEmpty())
        return;

    QModelIndex index = list.first();
    if (!index.isValid())
        return;

    index = m_openPagesModel->index((index.row() + summand + pageCount) % pageCount, 0);
    if (index.isValid()) {
        m_openPagesWidget->setCurrentIndex(index);
        m_openPagesWidget->scrollTo(index, QAbstractItemView::PositionAtCenter);
    }
}
