#pragma once

#include <QByteArray>
#include <QList>
#include "NetworkData.h"

class PacketStreamParser
{
public:
    struct ParseResult {
        bool valid = false;
        NetworkPacket::UnpackResult packet;
    };

    QList<ParseResult> appendAndParse(const QByteArray& bytes);
    void clear();

private:
    QByteArray m_buffer;
};
