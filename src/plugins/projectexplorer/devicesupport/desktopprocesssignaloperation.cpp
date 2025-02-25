// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "desktopprocesssignaloperation.h"

#include <app/app_version.h>

#include <utils/winutils.h>
#include <utils/fileutils.h>
#include <utils/processinfo.h>

#include <QCoreApplication>
#include <QDir>
#include <QProcess>

#ifdef Q_OS_WIN
#include <windows.h>
#ifndef PROCESS_SUSPEND_RESUME
#define PROCESS_SUSPEND_RESUME 0x0800
#endif // PROCESS_SUSPEND_RESUME
#else // Q_OS_WIN
#include <errno.h>
#include <signal.h>
#endif // else Q_OS_WIN

using namespace Utils;

namespace ProjectExplorer {

void DesktopProcessSignalOperation::killProcess(qint64 pid)
{
    killProcessSilently(pid);
    emit finished(m_errorMessage);
}

void DesktopProcessSignalOperation::killProcess(const QString &filePath)
{
    m_errorMessage.clear();
    const QList<ProcessInfo> processInfoList = ProcessInfo::processInfoList();
    for (const ProcessInfo &processInfo : processInfoList) {
        if (processInfo.commandLine == filePath)
            killProcessSilently(processInfo.processId);
    }
    emit finished(m_errorMessage);
}

void DesktopProcessSignalOperation::interruptProcess(qint64 pid)
{
    m_errorMessage.clear();
    interruptProcessSilently(pid);
    emit finished(m_errorMessage);
}

void DesktopProcessSignalOperation::interruptProcess(const QString &filePath)
{
    m_errorMessage.clear();
    const QList<ProcessInfo> processInfoList = ProcessInfo::processInfoList();
    for (const ProcessInfo &processInfo : processInfoList) {
        if (processInfo.commandLine == filePath)
            interruptProcessSilently(processInfo.processId);
    }
    emit finished(m_errorMessage);
}

void DesktopProcessSignalOperation::appendMsgCannotKill(qint64 pid, const QString &why)
{
    if (!m_errorMessage.isEmpty())
        m_errorMessage += QChar::fromLatin1('\n');
    m_errorMessage += tr("Cannot kill process with pid %1: %2").arg(pid).arg(why);
    m_errorMessage += QLatin1Char(' ');
}

void DesktopProcessSignalOperation::appendMsgCannotInterrupt(qint64 pid, const QString &why)
{
    if (!m_errorMessage.isEmpty())
        m_errorMessage += QChar::fromLatin1('\n');
    m_errorMessage += tr("Cannot interrupt process with pid %1: %2").arg(pid).arg(why);
    m_errorMessage += QLatin1Char(' ');
}

void DesktopProcessSignalOperation::killProcessSilently(qint64 pid)
{
#ifdef Q_OS_WIN
    const DWORD rights = PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION
            |PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ
            |PROCESS_DUP_HANDLE|PROCESS_TERMINATE|PROCESS_CREATE_THREAD|PROCESS_SUSPEND_RESUME;
    if (const HANDLE handle = OpenProcess(rights, FALSE, DWORD(pid))) {
        if (!TerminateProcess(handle, UINT(-1)))
            appendMsgCannotKill(pid, winErrorMessage(GetLastError()));
        CloseHandle(handle);
    } else {
        appendMsgCannotKill(pid, tr("Cannot open process."));
    }
#else
    if (pid <= 0)
        appendMsgCannotKill(pid, tr("Invalid process id."));
    else if (kill(pid, SIGKILL))
        appendMsgCannotKill(pid, QString::fromLocal8Bit(strerror(errno)));
#endif // Q_OS_WIN
}

void DesktopProcessSignalOperation::interruptProcessSilently(qint64 pid)
{
#ifdef Q_OS_WIN
    enum SpecialInterrupt { NoSpecialInterrupt, Win32Interrupt, Win64Interrupt };

    bool is64BitSystem = is64BitWindowsSystem();
    SpecialInterrupt si = NoSpecialInterrupt;
    if (is64BitSystem)
        si = is64BitWindowsBinary(m_debuggerCommand) ? Win64Interrupt : Win32Interrupt;
    /*
    Windows 64 bit has a 32 bit subsystem (WOW64) which makes it possible to run a
    32 bit application inside a 64 bit environment.
    When GDB is used DebugBreakProcess must be called from the same system (32/64 bit) running
    the inferior. If CDB is used we could in theory break wow64 processes,
    but the break is actually a wow64 breakpoint. CDB is configured to ignore these
    breakpoints, because they also appear on module loading.
    Therefore we need helper executables (win(32/64)interrupt.exe) on Windows 64 bit calling
    DebugBreakProcess from the correct system.

    DebugBreak matrix for windows

    Api = UseDebugBreakApi
    Win64 = UseWin64InterruptHelper
    Win32 = UseWin32InterruptHelper
    N/A = This configuration is not possible

          | Windows 32bit   | Windows 64bit
          | QtCreator 32bit | QtCreator 32bit                   | QtCreator 64bit
          | Inferior 32bit  | Inferior 32bit  | Inferior 64bit  | Inferior 32bit  | Inferior 64bit
----------|-----------------|-----------------|-----------------|-----------------|----------------
CDB 32bit | Api             | Api             | N/A             | Win32           | N/A
    64bit | N/A             | Win64           | Win64           | Api             | Api
----------|-----------------|-----------------|-----------------|-----------------|----------------
GDB 32bit | Api             | Api             | N/A             | Win32           | N/A
    64bit | N/A             | N/A             | Win64           | N/A             | Api
----------|-----------------|-----------------|-----------------|-----------------|----------------

    */
    HANDLE inferior = NULL;
    do {
        const DWORD rights = PROCESS_QUERY_INFORMATION|PROCESS_SET_INFORMATION
                |PROCESS_VM_OPERATION|PROCESS_VM_WRITE|PROCESS_VM_READ
                |PROCESS_DUP_HANDLE|PROCESS_TERMINATE|PROCESS_CREATE_THREAD|PROCESS_SUSPEND_RESUME;
        inferior = OpenProcess(rights, FALSE, pid);
        if (inferior == NULL) {
            appendMsgCannotInterrupt(pid, tr("Cannot open process: %1")
                                     + winErrorMessage(GetLastError()));
            break;
        }
        bool creatorIs64Bit = is64BitWindowsBinary(
            FilePath::fromUserInput(QCoreApplication::applicationFilePath()));
        if (!is64BitSystem
                || si == NoSpecialInterrupt
                || (si == Win64Interrupt && creatorIs64Bit)
                || (si == Win32Interrupt && !creatorIs64Bit)) {
            if (!DebugBreakProcess(inferior)) {
                appendMsgCannotInterrupt(pid, tr("DebugBreakProcess failed:")
                                          + QLatin1Char(' ') + winErrorMessage(GetLastError()));
            }
        } else if (si == Win32Interrupt || si == Win64Interrupt) {
            QString executable = QCoreApplication::applicationDirPath();
            executable += si == Win32Interrupt
                    ? QLatin1String("/win32interrupt.exe")
                    : QLatin1String("/win64interrupt.exe");
            if (!QFile::exists(executable)) {
                appendMsgCannotInterrupt(pid,
                                         tr("%1 does not exist. If you built %2 "
                                            "yourself, check out https://code.qt.io/cgit/"
                                            "qt-creator/binary-artifacts.git/.")
                                             .arg(QDir::toNativeSeparators(executable),
                                                  QString(Core::Constants::IDE_DISPLAY_NAME)));
            }
            switch (QProcess::execute(executable, QStringList(QString::number(pid)))) {
            case -2:
                appendMsgCannotInterrupt(pid, tr(
                            "Cannot start %1. Check src\\tools\\win64interrupt\\win64interrupt.c "
                            "for more information.").arg(QDir::toNativeSeparators(executable)));
                break;
            case 0:
                break;
            default:
                appendMsgCannotInterrupt(pid, QDir::toNativeSeparators(executable)
                                         + QLatin1Char(' ') + tr("could not break the process."));
                break;
            }
        }
    } while (false);
    if (inferior != NULL)
        CloseHandle(inferior);
#else
    if (pid <= 0)
        appendMsgCannotInterrupt(pid, tr("Invalid process id."));
    else if (kill(pid, SIGINT))
        appendMsgCannotInterrupt(pid, QString::fromLocal8Bit(strerror(errno)));
#endif // Q_OS_WIN
}

} // namespace ProjectExplorer
