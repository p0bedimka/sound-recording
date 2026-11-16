#include "Core/Inc/WavWriter.h"

namespace {
void writeU16LE(QFile& file, quint16 value) {
    const char bytes[] = {
        static_cast<char>(value & 0xFF),
        static_cast<char>((value >> 8) & 0xFF)
    };
    file.write(bytes, sizeof(bytes));
}

void writeU32LE(QFile& file, quint32 value) {
    const char bytes[] = {
        static_cast<char>(value & 0xFF),
        static_cast<char>((value >> 8) & 0xFF),
        static_cast<char>((value >> 16) & 0xFF),
        static_cast<char>((value >> 24) & 0xFF)
    };
    file.write(bytes, sizeof(bytes));
}
}

WavWriter::WavWriter() = default;
WavWriter::~WavWriter() {
    close();
}

bool WavWriter::open(const QString& filePath) {
    close();
    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }
    m_dataBytes = 0;
    m_sampleCount = 0;
    m_headerWritten = false;

    const quint32 riffSize = 36;
    const quint16 audioFormat = 1;
    const quint16 channels = 1;
    const quint32 sampleRate = 16000;
    const quint32 byteRate = 32000;
    const quint16 blockAlign = 2;
    const quint16 bitsPerSample = 16;
    const quint32 dataChunkSize = 0;

    m_file.write("RIFF", 4);
    writeU32LE(m_file, riffSize);
    m_file.write("WAVE", 4);
    m_file.write("fmt ", 4);
    writeU32LE(m_file, 16);
    writeU16LE(m_file, audioFormat);
    writeU16LE(m_file, channels);
    writeU32LE(m_file, sampleRate);
    writeU32LE(m_file, byteRate);
    writeU16LE(m_file, blockAlign);
    writeU16LE(m_file, bitsPerSample);
    m_file.write("data", 4);
    writeU32LE(m_file, dataChunkSize);
    m_headerWritten = true;
    return true;
}

void WavWriter::close() {
    if (m_file.isOpen()) {
        flushHeader();
        m_file.close();
    }
}

bool WavWriter::isOpen() const {
    return m_file.isOpen();
}

void WavWriter::writePayload(const QByteArray& payload) {
    if (!m_file.isOpen()) {
        return;
    }
    if (payload.isEmpty()) {
        return;
    }
    m_file.write(payload.constData(), payload.size());
    m_dataBytes += payload.size();
    m_sampleCount += payload.size() / 2;
}

void WavWriter::writeSilence(quint32 sampleCount) {
    if (sampleCount == 0 || !m_file.isOpen()) {
        return;
    }
    QByteArray silence(sampleCount * 2, char(0));
    writePayload(silence);
}

void WavWriter::flushHeader() {
    if (!m_file.isOpen() || !m_headerWritten) {
        return;
    }

    const quint32 riffSize = 36 + static_cast<quint32>(m_dataBytes);
    const quint32 dataSize = static_cast<quint32>(m_dataBytes);

    m_file.seek(4);
    writeU32LE(m_file, riffSize);

    m_file.seek(40);
    writeU32LE(m_file, dataSize);
    m_file.flush();
}

qint64 WavWriter::dataSize() const {
    return m_dataBytes;
}

qint64 WavWriter::sampleCount() const {
    return m_sampleCount;
}
