#pragma once

#include <QByteArray>
#include <QFile>
#include <QTextStream>

#include "FrameParser.h"

class PacketLog {
public:
    PacketLog();
    ~PacketLog();

    bool open(const QString& filePath);
    void close();
    bool isOpen() const;

    void writeHeader(const QString& portName,
                     const QString& portDescription,
                     const QString& sessionDir,
                     const QString& lineSettings);
    void writeFrame(const Core::AudioFrame& frame,
                    quint32 expectedSeq,
                    quint32 gap,
                    const QString& mode = QStringLiteral("full"));
    void writeFooter(qint64 durationMs,
                     quint64 frames,
                     quint64 samples,
                     quint64 gaps,
                     quint64 crcErrors,
                     quint64 resyncs,
                     quint64 drops);

private:
    QFile m_file;
    QTextStream m_stream;
    bool m_isOpen = false;
};
