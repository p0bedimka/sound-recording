#include "Core/Inc/FrameParser.h"
#include "Core/Inc/Crc32Mpeg2.h"
#include "Core/Inc/Protocol.h"

#include <QDateTime>

namespace {
quint16 readU16LE(const QByteArray& data, int offset) {
    return static_cast<quint16>(static_cast<quint8>(data[offset]) |
                                (static_cast<quint16>(static_cast<quint8>(data[offset + 1])) << 8));
}

quint32 readU32LE(const QByteArray& data, int offset) {
    quint32 value = static_cast<quint32>(static_cast<quint8>(data[offset]));
    value |= static_cast<quint32>(static_cast<quint8>(data[offset + 1])) << 8;
    value |= static_cast<quint32>(static_cast<quint8>(data[offset + 2])) << 16;
    value |= static_cast<quint32>(static_cast<quint8>(data[offset + 3])) << 24;
    return value;
}
}

namespace Core {
FrameParser::FrameParser() = default;

QVector<AudioFrame> FrameParser::process(const QByteArray& incoming) {
    if (incoming.isEmpty()) {
        return {};
    }

    m_accumulator.append(incoming);
    constexpr int maxAccumulator = 64 * 1024;
    if (m_accumulator.size() > maxAccumulator) {
        m_accumulator.remove(0, m_accumulator.size() - maxAccumulator);
        ++m_resyncs;
    }

    QVector<AudioFrame> out;
    while (!m_accumulator.isEmpty()) {
        int sync = -1;
        for (int i = 0; i + 1 < m_accumulator.size(); ++i) {
            if (static_cast<quint8>(m_accumulator[i]) == kSync0 && static_cast<quint8>(m_accumulator[i + 1]) == kSync1) {
                sync = i;
                break;
            }
        }
        if (sync < 0) {
            if (m_accumulator.size() > 1) {
                m_accumulator.remove(0, m_accumulator.size() - 1);
            }
            break;
        }
        if (sync > 0) {
            m_accumulator.remove(0, sync);
            ++m_resyncs;
            continue;
        }

        if (m_accumulator.size() < kHeaderSize) {
            break;
        }
        if (static_cast<quint8>(m_accumulator[2]) != kVersion || static_cast<quint8>(m_accumulator[3]) != kAudioType) {
            m_accumulator.remove(0, 1);
            ++m_resyncs;
            continue;
        }

        const quint16 length = readU16LE(m_accumulator, 8);
        if (length != kAudioPayloadSize) {
            m_accumulator.remove(0, 1);
            ++m_resyncs;
            continue;
        }

        const int frameSize = kAudioFrameSize;
        if (m_accumulator.size() < frameSize) {
            break;
        }

        QByteArray frame = m_accumulator.left(frameSize);
        const QByteArray crcInput = frame.mid(2, 8 + kAudioPayloadSize);
        const quint32 expectedCrc = readU32LE(frame, 10 + kAudioPayloadSize);
        const quint32 actualCrc = Core::crc32_mpeg2(reinterpret_cast<const uint8_t*>(crcInput.constData()), crcInput.size());

        if (expectedCrc == actualCrc) {
            AudioFrame audioFrame;
            audioFrame.seq = readU32LE(frame, 4);
            audioFrame.payload = frame.mid(10, kAudioPayloadSize);
            audioFrame.crc = expectedCrc;
            audioFrame.rxTimeMs = QDateTime::currentMSecsSinceEpoch();
            out.append(audioFrame);
            m_accumulator.remove(0, frameSize);
        } else {
            ++m_crcErrors;
            ++m_resyncs;
            m_accumulator.remove(0, 1);
        }
    }

    return out;
}

void FrameParser::reset() {
    m_accumulator.clear();
    m_crcErrors = 0;
    m_resyncs = 0;
}

quint64 FrameParser::crcErrors() const {
    return m_crcErrors;
}

quint64 FrameParser::resyncs() const {
    return m_resyncs;
}
}
