#include "App/ui/MainWindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , m_session(this) {
    setWindowTitle(QStringLiteral("UART Audio Recorder"));
    resize(840, 280);

    QWidget* central = new QWidget(this);
    setCentralWidget(central);

    auto* layout = new QVBoxLayout(central);
    auto* portLayout = new QHBoxLayout();
    m_portCombo = new QComboBox(this);
    m_refreshButton = new QPushButton(QStringLiteral("Обновить"), this);
    m_recordButton = new QPushButton(QStringLiteral("Начать запись"), this);
    m_recordButton->setCheckable(true);
    portLayout->addWidget(m_portCombo);
    portLayout->addWidget(m_refreshButton);
    portLayout->addWidget(m_recordButton);
    layout->addLayout(portLayout);

    m_stateLabel = new QLabel(QStringLiteral("READY"), this);
    m_stateLabel->setStyleSheet("font-weight: bold; font-size: 16px;");
    layout->addWidget(m_stateLabel);

    m_sessionLabel = new QLabel(QStringLiteral("Папка сессии: не создана"), this);
    layout->addWidget(m_sessionLabel);

    m_statsLabel = new QLabel(QStringLiteral("Статистика: ожидание порта"), this);
    m_statsLabel->setWordWrap(true);
    layout->addWidget(m_statsLabel);

    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::refreshPorts);
    connect(m_recordButton, &QPushButton::clicked, this, &MainWindow::toggleRecording);
    connect(&m_serialLink, &SerialLink::readyRead, &m_session, &RecordSession::onPortReadyRead);
    connect(&m_session, &RecordSession::stateChanged, this, &MainWindow::onStateChanged);
    connect(&m_session, &RecordSession::statsUpdated, this, &MainWindow::onStatsUpdated);
    connect(&m_session, &RecordSession::errorOccurred, this, &MainWindow::onErrorOccurred);
    m_session.setSerialLink(&m_serialLink);

    m_refreshTimer.setInterval(1000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &MainWindow::refreshPorts);
    m_refreshTimer.start();

    refreshPorts();
    updateControls();
}

void MainWindow::refreshPorts() {
    const QStringList ports = SerialLink::availablePorts();
    const QString current = m_portCombo->currentData().toString();
    m_portCombo->clear();
    for (const QString& port : ports) {
        m_portCombo->addItem(port, port);
    }
    if (!current.isEmpty()) {
        const int index = m_portCombo->findData(current);
        if (index >= 0) {
            m_portCombo->setCurrentIndex(index);
        }
    }
    updateControls();
}

void MainWindow::toggleRecording() {
    const auto state = m_session.state();
    if (state == RecordSession::State::Starting || state == RecordSession::State::Recording || state == RecordSession::State::Stopping) {
        m_session.stopRecording();
        return;
    }

    const QString selected = m_portCombo->currentData().toString();
    if (selected.isEmpty()) {
        onErrorOccurred(QStringLiteral("Сначала выберите COM-порт."));
        return;
    }

    if (!m_session.startRecording(selected)) {
        return;
    }
    m_recordButton->setText(QStringLiteral("Остановить запись"));
    updateControls();
}

void MainWindow::onStateChanged(const QString& stateName) {
    m_stateLabel->setText(stateName);
    if (stateName == QLatin1String("READY")) {
        m_recordButton->setText(QStringLiteral("Начать запись"));
    } else if (stateName == QLatin1String("RECORDING") || stateName == QLatin1String("STARTING") || stateName == QLatin1String("STOPPING")) {
        m_recordButton->setText(QStringLiteral("Остановить запись"));
    }
    updateControls();
}

void MainWindow::onStatsUpdated(const QString& summary,
                               qint64 frames,
                               qint64 samples,
                               qint64 bytes,
                               qint64 gaps,
                               qint64 crcErrors,
                               qint64 resyncs,
                               qint64 elapsedMs,
                               qint64 wavSeconds) {
    const QString text = QStringLiteral("%1 | frames=%2 samples=%3 bytes=%4 gaps=%5 crc=%6 resync=%7 elapsed=%8s wav=%9s")
        .arg(summary)
        .arg(frames)
        .arg(samples)
        .arg(bytes)
        .arg(gaps)
        .arg(crcErrors)
        .arg(resyncs)
        .arg(static_cast<double>(elapsedMs) / 1000.0, 0, 'f', 2)
        .arg(static_cast<double>(wavSeconds), 0, 'f', 2);
    m_statsLabel->setText(text);
    if (!m_session.sessionDirectory().isEmpty()) {
        m_sessionLabel->setText(QStringLiteral("Папка сессии: %1").arg(m_session.sessionDirectory()));
    }
}

void MainWindow::onErrorOccurred(const QString& message) {
    m_stateLabel->setText(QStringLiteral("ERROR"));
    m_statsLabel->setText(message);
    m_recordButton->setText(QStringLiteral("Начать запись"));
    updateControls();
}

void MainWindow::updateControls() {
    const bool hasPort = !m_portCombo->currentData().toString().isEmpty();
    const auto state = m_session.state();
    const bool isBusy = state == RecordSession::State::Starting || state == RecordSession::State::Recording || state == RecordSession::State::Stopping;
    m_recordButton->setEnabled(hasPort || isBusy);
    m_portCombo->setEnabled(!isBusy);
    m_refreshButton->setEnabled(true);
    if (!hasPort) {
        m_recordButton->setEnabled(false);
    }
}
