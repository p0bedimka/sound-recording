#pragma once

#include <QObject>
#include <QByteArray>
#include <QStringList>
#include <QTimer>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

class SerialLink : public QObject {
    Q_OBJECT
public:
    explicit SerialLink(QObject* parent = nullptr);

    static QStringList availablePorts();
    bool openPort(const QString& portName);
    void closePort();
    bool isOpen() const;
    QString portName() const;
    QByteArray readData();
    qint64 writeData(const QByteArray& data);

signals:
    void readyRead();

private slots:
    void pollForInput();

private:
    QString m_portName;
    QByteArray m_rxBuffer;
    QTimer m_pollTimer;
#ifdef Q_OS_WIN
    HANDLE m_handle = INVALID_HANDLE_VALUE;
#endif
};
