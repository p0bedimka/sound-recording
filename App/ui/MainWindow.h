#pragma once

#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QTimer>

#include "Core/Inc/RecordSession.h"
#include "Core/Inc/SerialLink.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void refreshPorts();
    void toggleRecording();
    void onStateChanged(const QString& stateName);
    void onStatsUpdated(const QString& summary,
                        qint64 frames,
                        qint64 samples,
                        qint64 bytes,
                        qint64 gaps,
                        qint64 crcErrors,
                        qint64 resyncs,
                        qint64 elapsedMs,
                        qint64 wavSeconds);
    void onErrorOccurred(const QString& message);

private:
    void updateControls();

private:
    SerialLink m_serialLink;
    RecordSession m_session;
    QComboBox* m_portCombo = nullptr;
    QPushButton* m_refreshButton = nullptr;
    QPushButton* m_recordButton = nullptr;
    QLabel* m_stateLabel = nullptr;
    QLabel* m_statsLabel = nullptr;
    QLabel* m_sessionLabel = nullptr;
    QTimer m_refreshTimer;
};
