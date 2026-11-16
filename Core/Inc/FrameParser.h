#pragma once

#include <QByteArray>
#include <QVector>
#include <QtGlobal>

namespace Core {
struct AudioFrame {
    quint32 seq = 0;
    QByteArray payload;
    quint32 crc = 0;
    qint64 rxTimeMs = 0;
};

class FrameParser {
public:
    FrameParser();
    QVector<AudioFrame> process(const QByteArray& incoming);
    void reset();

    quint64 crcErrors() const;
    quint64 resyncs() const;

private:
    QByteArray m_accumulator;
    quint64 m_crcErrors = 0;
    quint64 m_resyncs = 0;
};
}
