#include <QByteArray>
#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include "Core/Inc/Crc32Mpeg2.h"
#include "Core/Inc/FrameParser.h"
#include "Core/Inc/Protocol.h"
#include "Core/Inc/WavWriter.h"

class CoreTests : public QObject {
    Q_OBJECT
private slots:
    void crcVectors() {
        const QByteArray payload1 = QByteArray::fromHex("02 01 00 00 00 00 00 00");
        const quint32 crc1 = Core::crc32_mpeg2(reinterpret_cast<const uint8_t*>(payload1.constData()), payload1.size());
        QCOMPARE(crc1, 0x91106ADCu);

        const QByteArray payload2 = QByteArray::fromHex("02 02 00 00 00 00 00 00");
        const quint32 crc2 = Core::crc32_mpeg2(reinterpret_cast<const uint8_t*>(payload2.constData()), payload2.size());
        QCOMPARE(crc2, 0x40E9D2EFu);

        const QByteArray payload3 = QByteArray("123456789");
        const quint32 crc3 = Core::crc32_mpeg2(reinterpret_cast<const uint8_t*>(payload3.constData()), payload3.size());
        QCOMPARE(crc3, 0x0376E6E7u);
    }

    void commandFrameTest() {
        const auto frame = Core::buildCommand(Core::kStartType, 0);
        QVERIFY(frame.size() == 14);
        QVERIFY(frame.startsWith(QByteArray::fromHex("A5 5A")));
        QCOMPARE(static_cast<quint8>(frame[2]), static_cast<quint8>(Core::kVersion));
        QCOMPARE(static_cast<quint8>(frame[3]), static_cast<quint8>(Core::kStartType));
    }

    void audioPayloadIsPreserved() {
        QByteArray payload;
        const quint16 samples[] = {0x0000, 0x0001, 0x7FFF, 0x8000, 0xFFFF};
        for (int i = 0; i < 80; ++i) {
            const quint16 sample = samples[i % 5];
            payload.append(static_cast<char>(sample & 0xFF));
            payload.append(static_cast<char>((sample >> 8) & 0xFF));
        }

        QByteArray frame = QByteArray::fromHex("A5 5A 02 10 00 00 00 00 A0 00");
        frame.append(payload);
        const QByteArray crcInput = frame.mid(2);
        const quint32 crc = Core::crc32_mpeg2(
            reinterpret_cast<const uint8_t*>(crcInput.constData()),
            static_cast<size_t>(crcInput.size()));
        for (int shift = 0; shift < 4; ++shift) {
            frame.append(static_cast<char>((crc >> (shift * 8)) & 0xFF));
        }

        Core::FrameParser parser;
        const auto frames = parser.process(frame);
        QCOMPARE(frames.size(), 1);
        QCOMPARE(frames.first().payload, payload);
    }

    void wavHeaderTest() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString path = directory.filePath(QStringLiteral("recording.wav"));
        WavWriter writer;
        QVERIFY(writer.open(path));
        writer.writePayload(QByteArray::fromHex("34 12"));
        writer.close();

        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QCOMPARE(file.read(44), QByteArray::fromHex(
            "52 49 46 46 26 00 00 00 57 41 56 45 66 6D 74 20 "
            "10 00 00 00 01 00 01 00 80 3E 00 00 00 7D 00 00 "
            "02 00 10 00 64 61 74 61 02 00 00 00"));
        QCOMPARE(file.readAll(), QByteArray::fromHex("34 12"));
    }
};

QTEST_MAIN(CoreTests)
#include "core_tests.moc"
