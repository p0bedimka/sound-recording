#include "Core/Inc/RecordSession.h"
#include "Core/Inc/Protocol.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>

namespace {
quint32 moduloDiff(quint32 current, quint32 expected) {
    if (current >= expected) {
        return current - expected;
    }
    return (0xFFFFFFFFu - expected) + current + 1u;
}
}

RecordSession::RecordSession(QObject* parent)
    : QObject(parent) {
    connect(&m_stopDrainTimer, &QTimer::timeout, this, &RecordSession::onStopDrainTimeout);
    m_stopDrainTimer.setSingleShot(true);
    m_stopDrainTimer.setInterval(300);
}

RecordSession::~RecordSession() {
    finalizeSession();
}

void RecordSession::setSerialLink(SerialLink* serialLink) {
    m_serialLink = serialLink;
}

bool RecordSession::startRecording(const QString& portName) {
    if (m_isRecording) {
        return false;
    }
    if (!m_serialLink) {
        emit errorOccurred("No serial port attached.");
        return false;
    }
    if (!m_serialLink->openPort(portName)) {
        emit errorOccurred(QStringLiteral("Failed to open %1").arg(portName));
        setState(State::Error);
        return false;
    }

    m_portName = portName;
    if (!createSessionDirectory()) {
        emit errorOccurred(QStringLiteral("Failed to create session directory for %1").arg(portName));
        setState(State::Error);
        return false;
    }

    m_wavWriter.open(m_sessionDir + "/recording.wav");
    m_packetLog.open(m_sessionDir + "/packets.log");
    m_packetLog.writeHeader(portName,
                            QStringLiteral("USB serial interface"),
                            m_sessionDir,
                            QStringLiteral("921600 8N1 no flow control"));

    m_expectedSeq = 0;
    m_lastHandledSeq = 0;
    m_frames = 0;
    m_samples = 0;
    m_bytes = 0;
    m_gaps = 0;
    m_crcErrors = 0;
    m_resyncs = 0;
    m_drops = 0;
    m_startTimeMs = QDateTime::currentMSecsSinceEpoch();
    m_lastTickMs = m_startTimeMs;
    m_isRecording = true;
    setState(State::Starting);
    sendCommand(Core::kStartType);
    emitStats();
    return true;
}

void RecordSession::stopRecording() {
    if (!m_isRecording) {
        return;
    }
    setState(State::Stopping);
    sendCommand(Core::kStopType);
    m_stopDrainTimer.start();
}

void RecordSession::onPortReadyRead() {
    if (!m_serialLink) {
        return;
    }
    const QByteArray data = m_serialLink->readData();
    if (data.isEmpty()) {
        return;
    }

    const QVector<Core::AudioFrame> frames = m_parser.process(data);
    for (const auto& frame : frames) {
        processFrame(frame);
    }

    if (m_state == State::Starting || m_state == State::Recording || m_state == State::Stopping) {
        emitStats();
    }
}

QString RecordSession::sessionDirectory() const {
    return m_sessionDir;
}

RecordSession::State RecordSession::state() const {
    return m_state;
}

qint64 RecordSession::frames() const {
    return static_cast<qint64>(m_frames);
}

qint64 RecordSession::samples() const {
    return static_cast<qint64>(m_samples);
}

qint64 RecordSession::gaps() const {
    return static_cast<qint64>(m_gaps);
}

qint64 RecordSession::crcErrors() const {
    return static_cast<qint64>(m_crcErrors);
}

qint64 RecordSession::resyncs() const {
    return static_cast<qint64>(m_resyncs);
}

void RecordSession::onStopDrainTimeout() {
    finalizeSession();
    if (m_serialLink) {
        m_serialLink->closePort();
    }
    m_isRecording = false;
    setState(State::Ready);
    emitStats();
}

void RecordSession::setState(State newState) {
    if (m_state == newState) {
        return;
    }
    m_state = newState;
    emit stateChanged(QString::fromLatin1([&]() {
        switch (newState) {
        case State::Ready: return "READY";
        case State::Opening: return "OPENING";
        case State::Starting: return "STARTING";
        case State::Recording: return "RECORDING";
        case State::Stopping: return "STOPPING";
        case State::Error: return "ERROR";
        }
        return "READY";
    }()));
}

bool RecordSession::createSessionDirectory() {
    const QString basePath = QCoreApplication::applicationDirPath() + "/Data";
    QDir dir(basePath);
    if (!dir.exists()) {
        dir.mkpath(basePath);
    }

    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    m_sessionDir = basePath + "/" + timestamp;
    int suffix = 1;
    while (QFileInfo(m_sessionDir).exists()) {
        m_sessionDir = QStringLiteral("%1/%2_%3").arg(basePath).arg(timestamp).arg(suffix, 2, 10, QLatin1Char('0'));
        ++suffix;
    }
    return QDir().mkpath(m_sessionDir);
}

void RecordSession::emitStats() {
    const qint64 elapsedMs = QDateTime::currentMSecsSinceEpoch() - static_cast<qint64>(m_startTimeMs);
    const qint64 wavSeconds = m_wavWriter.sampleCount() / 16000;
    const QString summary = QStringLiteral("frames=%1 gaps=%2 crc=%3 resync=%4 elapsed=%5s")
        .arg(m_frames)
        .arg(m_gaps)
        .arg(m_crcErrors)
        .arg(m_resyncs)
        .arg(static_cast<double>(elapsedMs) / 1000.0, 0, 'f', 2);
    emit statsUpdated(summary,
                      static_cast<qint64>(m_frames),
                      static_cast<qint64>(m_samples),
                      static_cast<qint64>(m_bytes),
                      static_cast<qint64>(m_gaps),
                      static_cast<qint64>(m_crcErrors),
                      static_cast<qint64>(m_resyncs),
                      elapsedMs,
                      wavSeconds);
}

void RecordSession::processFrame(const Core::AudioFrame& frame) {
    if (m_state == State::Starting) {
        setState(State::Recording);
    }
    const quint32 seq = frame.seq;
    const quint32 delta = moduloDiff(seq, m_expectedSeq);
    quint32 gapBlocks = 0;

    if (delta == 0) {
        gapBlocks = 0;
    } else if (delta > 0) {
        gapBlocks = delta;
    } else {
        gapBlocks = 0;
        ++m_drops;
        if (m_packetLog.isOpen()) {
            m_packetLog.writeFrame(frame, m_expectedSeq, 0, QStringLiteral("header"));
        }
        return;
    }

    if (gapBlocks > 0) {
        m_gaps += gapBlocks;
        writeGapSamples(gapBlocks);
        if (m_packetLog.isOpen()) {
            m_packetLog.writeFrame(frame, m_expectedSeq, gapBlocks, QStringLiteral("header"));
        }
    }

    m_wavWriter.writePayload(frame.payload);
    m_bytes += frame.payload.size();
    m_samples += frame.payload.size() / 2;
    ++m_frames;
    m_expectedSeq = (seq + 1u) & 0xFFFFFFFFu;
    m_lastHandledSeq = seq;

    if (m_packetLog.isOpen()) {
        m_packetLog.writeFrame(frame, m_expectedSeq, gapBlocks, QStringLiteral("full"));
    }
}

void RecordSession::finalizeSession() {
    if (!m_wavWriter.isOpen() && !m_packetLog.isOpen()) {
        return;
    }

    if (m_wavWriter.isOpen()) {
        m_wavWriter.close();
    }
    if (m_packetLog.isOpen()) {
        const qint64 durationMs = QDateTime::currentMSecsSinceEpoch() - static_cast<qint64>(m_startTimeMs);
        m_packetLog.writeFooter(durationMs,
                               m_frames,
                               m_samples,
                               m_gaps,
                               m_crcErrors,
                               m_resyncs,
                               m_drops);
        m_packetLog.close();
    }
}

void RecordSession::sendCommand(quint8 type) {
    if (!m_serialLink || !m_serialLink->isOpen()) {
        emit errorOccurred(QStringLiteral("Port is not open for command %1").arg(type));
        return;
    }
    const QByteArray packet = Core::buildCommand(type, 0);
    const qint64 written = m_serialLink->writeData(packet);
    if (written != packet.size()) {
        emit errorOccurred(QStringLiteral("Failed to send %1 command").arg(Core::describeProtocolState(type)));
    }
}

void RecordSession::writeGapSamples(quint32 blocks) {
    if (blocks == 0) {
        return;
    }
    const quint32 sampleCount = std::min<quint32>(blocks * 80u, 80u * 60u);
    m_wavWriter.writeSilence(sampleCount);
    m_samples += sampleCount;
}
