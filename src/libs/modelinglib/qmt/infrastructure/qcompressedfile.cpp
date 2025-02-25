// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "qcompressedfile.h"

#include "qmt/infrastructure/qmtassert.h"

// TODO create real compressed files that can be read by some archiver (gzip)

namespace qmt {

QCompressedDevice::QCompressedDevice(QObject *parent)
    : QIODevice(parent)
{
}

QCompressedDevice::QCompressedDevice(QIODevice *targetDevice, QObject *parent)
    : QIODevice(parent),
      m_targetDevice(targetDevice)
{
}

QCompressedDevice::~QCompressedDevice()
{
    flush();
}

void QCompressedDevice::setTargetDevice(QIODevice *targetDevice)
{
    m_targetDevice = targetDevice;
}

void QCompressedDevice::close()
{
    flush();
    QIODevice::close();
}

qint64 QCompressedDevice::readData(char *data, qint64 maxlen)
{
    QMT_ASSERT(m_targetDevice, return 0);
    QMT_ASSERT(m_targetDevice->isOpen(), return 0);
    QMT_ASSERT(m_targetDevice->openMode() == QIODevice::ReadOnly, return 0);

    if (m_bytesInBuffer == 0) {
        QByteArray compressedBuffer;
        int compressedLen = 0;
        if (m_targetDevice->read(reinterpret_cast<char *>(&compressedLen), sizeof(int)) != sizeof(int))
            return -1;
        compressedBuffer.resize(compressedLen);
        qint64 compressedBytes = m_targetDevice->read(compressedBuffer.data(), compressedLen);
        m_buffer = qUncompress((const uchar *) compressedBuffer.data(), compressedBytes);
        m_bytesInBuffer = m_buffer.size();
        if (m_bytesInBuffer == 0)
            return 0;
        m_indexInBuffer = 0;
    }
    qint64 n = std::min(maxlen, m_bytesInBuffer);
    memcpy(data, m_buffer.data() + m_indexInBuffer, n);
    m_bytesInBuffer -= n;
    m_indexInBuffer += n;
    return n;
}

qint64 QCompressedDevice::writeData(const char *data, qint64 len)
{
    QMT_ASSERT(m_targetDevice, return 0);
    QMT_ASSERT(m_targetDevice->isOpen(), return 0);
    QMT_ASSERT(m_targetDevice->openMode() == QIODevice::WriteOnly, return 0);

    m_buffer.append(data, len);
    if (m_buffer.size() > 1024*1024) {
        QByteArray compressedBuffer = qCompress(m_buffer);
        int compressedLen = static_cast<int>(compressedBuffer.size());
        if (m_targetDevice->write(reinterpret_cast<const char *>(&compressedLen), sizeof(int)) != sizeof(int))
            return -1;
        if (m_targetDevice->write(compressedBuffer.data(), compressedLen) != compressedBuffer.size())
            return -1;
        m_buffer.clear();
    }
    return len;
}

qint64 QCompressedDevice::flush()
{
    if (openMode() == QIODevice::WriteOnly && !m_buffer.isEmpty()) {
        QMT_ASSERT(m_targetDevice->isOpen(), return 0);
        QMT_ASSERT(m_targetDevice->openMode() == QIODevice::WriteOnly, return 0);
        QByteArray compressedBuffer = qCompress(m_buffer);
        int compressedLen = static_cast<int>(compressedBuffer.size());
        if (m_targetDevice->write(reinterpret_cast<const char *>(&compressedLen), sizeof(int)) != sizeof(int))
            return -1;
        return m_targetDevice->write(compressedBuffer.data(), compressedLen);
    }
    return 0;
}

} // namespace qmt
