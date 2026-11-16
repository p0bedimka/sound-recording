#pragma once

#include <QByteArray>
#include <QFile>
#include <QString>

class WavWriter {
public:
    WavWriter();
    ~WavWriter();

    bool open(const QString& filePath);
    void close();
    bool isOpen() const;

    void writePayload(const QByteArray& payload);
    void writeSilence(quint32 sampleCount);
    void flushHeader();

    qint64 dataSize() const;
    qint64 sampleCount() const;

private:
    QFile m_file;
    qint64 m_dataBytes = 0;
    qint64 m_sampleCount = 0;
    bool m_headerWritten = false;
};
