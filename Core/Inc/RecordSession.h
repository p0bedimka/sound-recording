#pragma once

#include <QObject>
#include <QTimer>

#include "FrameParser.h"
#include "PacketLog.h"
#include "SerialLink.h"
#include "WavWriter.h"

class RecordSession : public QObject {
    Q_OBJECT
public:
    enum class State {
        Ready,
        Opening,
        Starting,
        Recording,
        Stopping,
        Error
    };
    Q_ENUM(State)

    explicit RecordSession(QObject* parent = nullptr);
    ~RecordSession() override;

    void setSerialLink(SerialLink* serialLink);
    bool startRecording(const QString& portName);
    void stopRecording();
    void onPortReadyRead();
    QString sessionDirectory() const;
    State state() const;
    qint64 frames() const;
    qint64 samples() const;
    qint64 gaps() const;
    qint64 crcErrors() const;
    qint64 resyncs() const;

signals:
    void stateChanged(const QString& stateName);
    void statsUpdated(const QString& summary,
                      qint64 frames,
                      qint64 samples,
                      qint64 bytes,
                      qint64 gaps,
                      qint64 crcErrors,
                      qint64 resyncs,
                      qint64 elapsedMs,
                      qint64 wavSeconds);
    void errorOccurred(const QString& message);

private slots:
    void onStopDrainTimeout();

private:
    void setState(State newState);
    bool createSessionDirectory();
    void emitStats();
    void processFrame(const Core::AudioFrame& frame);
    void finalizeSession();
    void sendCommand(quint8 type);
    void writeGapSamples(quint32 blocks);

private:
    SerialLink* m_serialLink = nullptr;
    Core::FrameParser m_parser;
    WavWriter m_wavWriter;
    PacketLog m_packetLog;
    QTimer m_stopDrainTimer;
    QString m_portName;
    QString m_sessionDir;
    State m_state = State::Ready;
    quint32 m_expectedSeq = 0;
    quint32 m_lastHandledSeq = 0;
    quint64 m_frames = 0;
    quint64 m_samples = 0;
    quint64 m_bytes = 0;
    quint64 m_gaps = 0;
    quint64 m_crcErrors = 0;
    quint64 m_resyncs = 0;
    quint64 m_drops = 0;
    quint64 m_startTimeMs = 0;
    quint64 m_lastTickMs = 0;
    bool m_isRecording = false;
};
