#include "PacketStreamWriter.h"

#include <QMetaObject>

PacketStreamWriter::PacketStreamWriter(QObject* parent)
    : QObject(parent)
{
}

void PacketStreamWriter::enqueue(CmdType type, const QByteArray& body)
{
    m_sendQueue.enqueue(NetworkPacket::pack(type, body));
    QMetaObject::invokeMethod(this, "processQueue", Qt::QueuedConnection);
}

void PacketStreamWriter::processQueue()
{
    while (!m_sendQueue.isEmpty()) {
        emit writeToSocket(m_sendQueue.dequeue());
    }
}
