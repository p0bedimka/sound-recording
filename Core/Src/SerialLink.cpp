#include "Core/Inc/SerialLink.h"

#include <QDir>
#include <cmath>
#include <algorithm>

SerialLink::SerialLink(QObject* parent)
    : QObject(parent) {
    m_pollTimer.setInterval(20);
    connect(&m_pollTimer, &QTimer::timeout, this, &SerialLink::pollForInput);
}

QStringList SerialLink::availablePorts() {
    QStringList result;
#ifdef Q_OS_WIN
    for (int i = 1; i <= 256; ++i) {
        const QString portName = QStringLiteral("COM%1").arg(i);
        const QString fullName = QStringLiteral("\\\\.\\%1").arg(portName);
        const std::wstring wName = fullName.toStdWString();
        const HANDLE handle = CreateFileW(wName.c_str(),
                                         GENERIC_READ | GENERIC_WRITE,
                                         0,
                                         nullptr,
                                         OPEN_EXISTING,
                                         FILE_ATTRIBUTE_NORMAL,
                                         nullptr);
        if (handle != INVALID_HANDLE_VALUE) {
            result << portName;
            CloseHandle(handle);
        }
    }
#else
    const QStringList entries = QDir("/dev").entryList(QDir::System | QDir::Readable);
    for (const QString& entry : entries) {
        if (entry.startsWith("tty") || entry.startsWith("cu")) {
            result << QString("/dev/%1").arg(entry);
        }
    }
#endif
    return result;
}

bool SerialLink::openPort(const QString& portName) {
    closePort();
#ifdef Q_OS_WIN
    m_portName = portName;
    const QString fullName = QStringLiteral("\\\\.\\%1").arg(portName);
    const std::wstring wName = fullName.toStdWString();
    m_handle = CreateFileW(wName.c_str(),
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           nullptr,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           nullptr);
    if (m_handle == INVALID_HANDLE_VALUE) {
        return false;
    }
    DCB dcb{};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(m_handle, &dcb)) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }
    dcb.BaudRate = 921600;
    dcb.ByteSize = 8;
    dcb.StopBits = ONESTOPBIT;
    dcb.Parity = NOPARITY;
    dcb.fBinary = TRUE;
    dcb.fOutxCtsFlow = FALSE;
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fRtsControl = RTS_CONTROL_ENABLE;
    dcb.fAbortOnError = TRUE;
    if (!SetCommState(m_handle, &dcb)) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }
    COMMTIMEOUTS timeouts{};
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if (!SetCommTimeouts(m_handle, &timeouts)) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }
    m_pollTimer.start();
    return true;
#else
    Q_UNUSED(portName)
    return false;
#endif
}

void SerialLink::closePort() {
#ifdef Q_OS_WIN
    if (m_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }
    m_pollTimer.stop();
    m_rxBuffer.clear();
#endif
}

bool SerialLink::isOpen() const {
#ifdef Q_OS_WIN
    return m_handle != INVALID_HANDLE_VALUE;
#else
    return false;
#endif
}

QString SerialLink::portName() const {
    return m_portName;
}

QByteArray SerialLink::readData() {
    QByteArray out = m_rxBuffer;
    m_rxBuffer.clear();
    return out;
}

qint64 SerialLink::writeData(const QByteArray& data) {
#ifdef Q_OS_WIN
    if (m_handle == INVALID_HANDLE_VALUE) {
        return -1;
    }
    DWORD written = 0;
    if (!WriteFile(m_handle, data.constData(), static_cast<DWORD>(data.size()), &written, nullptr)) {
        return -1;
    }
    return static_cast<qint64>(written);
#else
    Q_UNUSED(data)
    return -1;
#endif
}

void SerialLink::pollForInput() {
#ifdef Q_OS_WIN
    if (m_handle == INVALID_HANDLE_VALUE) {
        return;
    }
    DWORD errors = 0;
    COMSTAT status{};
    if (!ClearCommError(m_handle, &errors, &status)) {
        return;
    }
    if (status.cbInQue == 0) {
        return;
    }
    const DWORD size = static_cast<DWORD>(std::min<quint64>(static_cast<quint64>(status.cbInQue), 4096ULL));
    QByteArray chunk(size, '\0');
    DWORD read = 0;
    if (!ReadFile(m_handle, chunk.data(), size, &read, nullptr)) {
        return;
    }
    if (read > 0) {
        m_rxBuffer.append(chunk.left(static_cast<int>(read)));
        emit readyRead();
    }
#endif
}
