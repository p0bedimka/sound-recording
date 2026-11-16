#pragma once

#include <QByteArray>
#include <QString>
#include <QtGlobal>

namespace Core {
constexpr quint8 kSync0 = 0xA5;
constexpr quint8 kSync1 = 0x5A;
constexpr quint8 kVersion = 0x02;
constexpr quint8 kAudioType = 0x10;
constexpr quint8 kStartType = 0x01;
constexpr quint8 kStopType = 0x02;
constexpr int kHeaderSize = 10;
constexpr int kAudioPayloadSize = 160;
constexpr int kAudioFrameSize = kHeaderSize + kAudioPayloadSize + 4;

QByteArray buildCommand(quint8 type, quint32 seq = 0);
QString describeProtocolState(int code);
}
