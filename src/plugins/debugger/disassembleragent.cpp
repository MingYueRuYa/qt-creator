// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "disassembleragent.h"

#include "breakhandler.h"
#include "debuggeractions.h"
#include "debuggerengine.h"
#include "debuggerinternalconstants.h"
#include "disassemblerlines.h"
#include "sourceutils.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>

#include <texteditor/textmark.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/aspects.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>

#include <QTextBlock>
#include <QDir>

using namespace Core;
using namespace TextEditor;

namespace Debugger::Internal {

///////////////////////////////////////////////////////////////////////
//
// DisassemblerBreakpointMarker
//
///////////////////////////////////////////////////////////////////////

// The red blob on the left side in the cpp editor.
class DisassemblerBreakpointMarker : public TextMark
{
public:
    DisassemblerBreakpointMarker(const Breakpoint &bp, int lineNumber)
        : TextMark(Utils::FilePath(), lineNumber, Constants::TEXT_MARK_CATEGORY_BREAKPOINT), m_bp(bp)
    {
        setIcon(bp->icon());
        setPriority(TextMark::NormalPriority);
    }

    bool isClickable() const final { return true; }

    void clicked() final
    {
        QTC_ASSERT(m_bp, return);
        m_bp->deleteGlobalOrThisBreakpoint();
    }

public:
    Breakpoint m_bp;
};

///////////////////////////////////////////////////////////////////////
//
// FrameKey
//
///////////////////////////////////////////////////////////////////////

class FrameKey
{
public:
    FrameKey() = default;
    inline bool matches(const Location &loc) const;

    QString functionName;
    QString fileName;
    quint64 startAddress = 0;
    quint64 endAddress = 0;
};

bool FrameKey::matches(const Location &loc) const
{
    return loc.address() >= startAddress
            && loc.address() <= endAddress
            && loc.fileName().toString() == fileName
            && loc.functionName() == functionName;
}

using CacheEntry = QPair<FrameKey, DisassemblerLines>;


///////////////////////////////////////////////////////////////////////
//
// DisassemblerAgentPrivate
//
///////////////////////////////////////////////////////////////////////

class DisassemblerAgentPrivate
{
public:
    DisassemblerAgentPrivate(DebuggerEngine *engine);
    ~DisassemblerAgentPrivate();
    void configureMimeType();
    int lineForAddress(quint64 address) const;

public:
    QPointer<TextDocument> document;
    Location location;
    QPointer<DebuggerEngine> engine;
    LocationMark locationMark;
    QList<DisassemblerBreakpointMarker *> breakpointMarks;
    QList<CacheEntry> cache;
    QString mimeType;
    bool resetLocationScheduled;
};

DisassemblerAgentPrivate::DisassemblerAgentPrivate(DebuggerEngine *engine)
  : document(nullptr),
    engine(engine),
    locationMark(engine, Utils::FilePath(), 0),
    mimeType("text/x-qtcreator-generic-asm"),
    resetLocationScheduled(false)
{}

DisassemblerAgentPrivate::~DisassemblerAgentPrivate()
{
    EditorManager::closeDocuments({document});
    document = nullptr;
    qDeleteAll(breakpointMarks);
}

int DisassemblerAgentPrivate::lineForAddress(quint64 address) const
{
    for (int i = 0, n = cache.size(); i != n; ++i) {
        const CacheEntry &entry = cache.at(i);
        if (entry.first.matches(location))
            return entry.second.lineForAddress(address);
    }
    return 0;
}


///////////////////////////////////////////////////////////////////////
//
// DisassemblerAgent
//
///////////////////////////////////////////////////////////////////////

/*!
    \class Debugger::Internal::DisassemblerAgent

     Objects from this class are created in response to user actions in
     the Gui for showing disassembled memory from the inferior. After creation
     it handles communication between the engine and the editor.
*/

DisassemblerAgent::DisassemblerAgent(DebuggerEngine *engine)
    : d(new DisassemblerAgentPrivate(engine))
{
    connect(&debuggerSettings()->intelFlavor, &Utils::BaseAspect::changed,
            this, &DisassemblerAgent::reload);
}

DisassemblerAgent::~DisassemblerAgent()
{
    delete d;
    d = nullptr;
}

int DisassemblerAgent::indexOf(const Location &loc) const
{
    for (int i = 0; i < d->cache.size(); i++)
        if (d->cache.at(i).first.matches(loc))
            return i;
    return -1;
}

void DisassemblerAgent::cleanup()
{
    d->cache.clear();
}

void DisassemblerAgent::scheduleResetLocation()
{
    d->resetLocationScheduled = true;
}

void DisassemblerAgent::resetLocation()
{
    if (!d->document)
        return;
    if (d->resetLocationScheduled) {
        d->resetLocationScheduled = false;
        d->document->removeMark(&d->locationMark);
    }
}

const Location &DisassemblerAgent::location() const
{
    return d->location;
}

void DisassemblerAgent::reload()
{
    d->cache.clear();
    d->engine->fetchDisassembler(this);
}

void DisassemblerAgent::setLocation(const Location &loc)
{
    d->location = loc;
    int index = indexOf(loc);
    if (index != -1) {
        // Refresh when not displaying a function and there is not sufficient
        // context left past the address.
        if (d->cache.at(index).first.endAddress - loc.address() < 24) {
            d->cache.removeAt(index);
            index = -1;
        }
    }
    if (index != -1) {
        const FrameKey &key = d->cache.at(index).first;
        const QString msg =
            QString("Using cached disassembly for 0x%1 (0x%2-0x%3) in \"%4\"/ \"%5\"")
                .arg(loc.address(), 0, 16)
                .arg(key.startAddress, 0, 16).arg(key.endAddress, 0, 16)
                .arg(loc.functionName(), loc.fileName().toUserOutput());
        d->engine->showMessage(msg);
        setContentsToDocument(d->cache.at(index).second);
        d->resetLocationScheduled = false; // In case reset from previous run still pending.
    } else {
        d->engine->fetchDisassembler(this);
    }
}

void DisassemblerAgentPrivate::configureMimeType()
{
    QTC_ASSERT(document, return);

    document->setMimeType(mimeType);

    Utils::MimeType mtype = Utils::mimeTypeForName(mimeType);
    if (mtype.isValid()) {
        const QList<IEditor *> editors = DocumentModel::editorsForDocument(document);
        for (IEditor *editor : editors)
            if (auto widget = TextEditorWidget::fromEditor(editor))
                widget->configureGenericHighlighter();
    } else {
        qWarning("Assembler mimetype '%s' not found.", qPrintable(mimeType));
    }
}

QString DisassemblerAgent::mimeType() const
{
    return d->mimeType;
}

void DisassemblerAgent::setMimeType(const QString &mt)
{
    if (mt == d->mimeType)
        return;
    d->mimeType = mt;
    if (d->document)
       d->configureMimeType();
}

void DisassemblerAgent::setContents(const DisassemblerLines &contents)
{
    QTC_ASSERT(d, return);
    if (contents.size()) {
        const quint64 startAddress = contents.startAddress();
        const quint64 endAddress = contents.endAddress();
        if (startAddress) {
            FrameKey key;
            key.fileName = d->location.fileName().toString();
            key.functionName = d->location.functionName();
            key.startAddress = startAddress;
            key.endAddress = endAddress;
            d->cache.append(CacheEntry(key, contents));
        }
    }
    setContentsToDocument(contents);
}

void DisassemblerAgent::setContentsToDocument(const DisassemblerLines &contents)
{
    QTC_ASSERT(d, return);
    if (!d->document) {
        QString titlePattern = "Disassembler";
        IEditor *editor = EditorManager::openEditorWithContents(
                Core::Constants::K_DEFAULT_TEXT_EDITOR_ID,
                &titlePattern);
        QTC_ASSERT(editor, return);
        if (auto widget = TextEditorWidget::fromEditor(editor)) {
            widget->setReadOnly(true);
            widget->setRequestMarkEnabled(true);
        }
        d->document = qobject_cast<TextDocument *>(editor->document());
        QTC_ASSERT(d->document, return);
        d->document->setTemporary(true);
        // FIXME: This is accumulating quite a bit out-of-band data.
        // Make that a proper TextDocument reimplementation.
        d->document->setProperty(Debugger::Constants::OPENED_BY_DEBUGGER, true);
        d->document->setProperty(Debugger::Constants::OPENED_WITH_DISASSEMBLY, true);
        d->document->setProperty(Debugger::Constants::DISASSEMBLER_SOURCE_FILE, d->location.fileName().toString());
        d->configureMimeType();
    } else {
        EditorManager::activateEditorForDocument(d->document);
    }

    d->document->setPlainText(contents.toString());

    d->document->setPreferredDisplayName(QString("Disassembler (%1)")
        .arg(d->location.functionName()));

    const Breakpoints bps = d->engine->breakHandler()->breakpoints();
    for (const Breakpoint &bp : bps)
        updateBreakpointMarker(bp);

    updateLocationMarker();
}

void DisassemblerAgent::updateLocationMarker()
{
    if (!d->document)
        return;

    int lineNumber = d->lineForAddress(d->location.address());
    if (d->location.needsMarker()) {
        d->document->removeMark(&d->locationMark);
        d->locationMark.updateLineNumber(lineNumber);
        d->document->addMark(&d->locationMark);
    }

    d->locationMark.updateIcon();

    // Center cursor.
    if (EditorManager::currentDocument() == d->document)
        if (auto textEditor = qobject_cast<BaseTextEditor *>(EditorManager::currentEditor()))
            textEditor->gotoLine(lineNumber);
}

void DisassemblerAgent::removeBreakpointMarker(const Breakpoint &bp)
{
    if (!d->document)
        return;

    for (DisassemblerBreakpointMarker *marker : std::as_const(d->breakpointMarks)) {
        if (marker->m_bp == bp) {
            d->breakpointMarks.removeOne(marker);
            d->document->removeMark(marker);
            delete marker;
            return;
        }
    }
}

void DisassemblerAgent::updateBreakpointMarker(const Breakpoint &bp)
{
    removeBreakpointMarker(bp);
    const quint64 address = bp->address();
    if (!address)
        return;

    int lineNumber = d->lineForAddress(address);
    if (!lineNumber)
        return;

    // HACK: If it's a FileAndLine breakpoint, and there's a source line
    // above, move the marker up there. That allows setting and removing
    // normal breakpoints from within the disassembler view.
    if (bp->type() == BreakpointByFileAndLine) {
        ContextData context = getLocationContext(d->document, lineNumber - 1);
        if (context.type == LocationByFile)
            --lineNumber;
    }

    auto marker = new DisassemblerBreakpointMarker(bp, lineNumber);
    d->breakpointMarks.append(marker);
    QTC_ASSERT(d->document, return);
    d->document->addMark(marker);
}

quint64 DisassemblerAgent::address() const
{
    return d->location.address();
}

} // Debugger::Internal
