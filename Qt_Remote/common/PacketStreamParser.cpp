#include "PacketStreamParser.h"

QList<PacketStreamParser::ParseResult> PacketStreamParser::appendAndParse(const QByteArray& bytes)
{
    QList<ParseResult> results;

    if (!bytes.isEmpty()) {
        m_buffer.append(bytes);
    }

    while (m_buffer.size() >= NetworkPacket::minSize()) {
        const int packetSize = NetworkPacket::totalSize(m_buffer);
        if (packetSize == 0) {
            m_buffer.remove(0, 1);
            continue;
        }

        if (m_buffer.size() < packetSize) {
            break;
        }

        const QByteArray onePacket = m_buffer.left(packetSize);
        auto unpack = NetworkPacket::unpack(onePacket);

        ParseResult r;
        r.valid = unpack.isValid;
        r.packet = unpack;
        results.append(r);

        m_buffer.remove(0, packetSize);
    }

    return results;
}

void PacketStreamParser::clear()
{
    m_buffer.clear();
}
