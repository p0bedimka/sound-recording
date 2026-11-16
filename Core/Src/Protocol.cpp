#include "Core/Inc/Protocol.h"
#include "Core/Inc/Crc32Mpeg2.h"

#include <QByteArray>
#include <QDataStream>

namespace Core {
QByteArray buildCommand(quint8 type, quint32 seq) {
    QByteArray frame;
    frame.reserve(14);
    frame.append(static_cast<char>(kSync0));
    frame.append(static_cast<char>(kSync1));
    frame.append(static_cast<char>(kVersion));
    frame.append(static_cast<char>(type));
    for (int shift = 0; shift < 4; ++shift) {
        frame.append(static_cast<char>((seq >> (shift * 8)) & 0xFFu));
    }
    quint16 length = 0;
    for (int shift = 0; shift < 2; ++shift) {
        frame.append(static_cast<char>((length >> (shift * 8)) & 0xFFu));
    }

    QByteArray crcInput = frame.mid(2, 8);
    const uint32_t crc = Core::crc32_mpeg2(reinterpret_cast<const uint8_t*>(crcInput.constData()), crcInput.size());
    const auto raw = static_cast<uint32_t>(crc);
    for (int shift = 0; shift < 4; ++shift) {
        frame.append(static_cast<char>((raw >> (shift * 8)) & 0xFFu));
    }
    return frame;
}

QString describeProtocolState(int code) {
    switch (code) {
    case 0x01: return QStringLiteral("START");
    case 0x02: return QStringLiteral("STOP");
    case 0x10: return QStringLiteral("AUDIO");
    default: return QStringLiteral("UNKNOWN");
    }
}
}
