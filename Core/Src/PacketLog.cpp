#include "Core/Inc/PacketLog.h"

#include <QDateTime>
#include <QRegularExpression>

PacketLog::PacketLog() = default;
PacketLog::~PacketLog() {
    close();
}

bool PacketLog::open(const QString& filePath) {
    close();
    m_file.setFileName(filePath);
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        return false;
    }
    m_stream.setDevice(&m_file);
    m_stream.setEncoding(QStringConverter::Utf8);
    m_isOpen = true;
    return true;
}

void PacketLog::close() {
    if (m_stream.device()) {
        m_stream.flush();
    }
    if (m_file.isOpen()) {
        m_file.close();
    }
    m_isOpen = false;
}

bool PacketLog::isOpen() const {
    return m_isOpen;
}

void PacketLog::writeHeader(const QString& portName,
                           const QString& portDescription,
                           const QString& sessionDir,
                           const QString& lineSettings) {
    if (!m_isOpen) {
        return;
    }
    const QDateTime now = QDateTime::currentDateTime();
    m_stream << "UART Audio Recorder session\n";
    m_stream << "started=" << now.toString(Qt::ISODateWithMs) << "\n";
    m_stream << "port=" << portName << "\n";
    m_stream << "description=" << portDescription << "\n";
    m_stream << "session_dir=" << sessionDir << "\n";
    m_stream << "line_settings=" << lineSettings << "\n";
    m_stream << "protocol=UART v2\n";
    m_stream << "format=PCM16 mono 16kHz\n\n";
    m_stream.flush();
}

void PacketLog::writeFrame(const Core::AudioFrame& frame,
                          quint32 expectedSeq,
                          quint32 gap,
                          const QString& mode) {
    if (!m_isOpen) {
        return;
    }
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 elapsedMs = static_cast<qint64>(frame.rxTimeMs > 0 ? frame.rxTimeMs : nowMs);
    const QByteArray dump = frame.payload.toHex(' ');
    m_stream << QStringLiteral("#%1 t=%2 seq=%3 exp=%4 gap=%5 len=%6 crc_rx=0x%7 crc_ok\n")
        .arg(frame.seq, 8, 10, QLatin1Char('0'))
        .arg(elapsedMs / 1000.0, 0, 'f', 3)
        .arg(frame.seq)
        .arg(expectedSeq)
        .arg(gap)
        .arg(frame.payload.size())
        .arg(frame.crc, 8, 16, QLatin1Char('0'));

    if (mode == QLatin1String("header")) {
        m_stream << "header_only\n";
    } else if (mode == QLatin1String("payload")) {
        m_stream << frame.payload.toHex(' ') << "\n";
    } else if (mode == QLatin1String("samples")) {
        const auto samples = frame.payload;
        for (int i = 0; i + 1 < samples.size(); i += 2) {
            const auto sample = static_cast<qint16>(static_cast<quint8>(samples[i]) |
                                                    (static_cast<quint16>(static_cast<quint8>(samples[i + 1])) << 8));
            m_stream << sample << ' ';
        }
        m_stream << "\n";
    } else {
        m_stream << dump << "\n";
    }
    m_stream.flush();
}

void PacketLog::writeFooter(qint64 durationMs,
                           quint64 frames,
                           quint64 samples,
                           quint64 gaps,
                           quint64 crcErrors,
                           quint64 resyncs,
                           quint64 drops) {
    if (!m_isOpen) {
        return;
    }
    m_stream << "\n";
    m_stream << "stopped=" << QDateTime::currentDateTime().toString(Qt::ISODateWithMs) << "\n";
    m_stream << "duration_ms=" << durationMs << "\n";
    m_stream << "frames=" << frames << "\n";
    m_stream << "samples=" << samples << "\n";
    m_stream << "gaps=" << gaps << "\n";
    m_stream << "crc_errors=" << crcErrors << "\n";
    m_stream << "resyncs=" << resyncs << "\n";
    m_stream << "uart_drops=" << drops << "\n";
    m_stream.flush();
}
