#include "ClientSession.h"

ClientSession::ClientSession(QTcpSocket* socket, QObject* parent)
    : QObject(parent), m_socket(socket), m_streamWriter(this)
{
    if (m_socket) {
        m_socket->setParent(this);
        connect(m_socket, &QTcpSocket::readyRead, this, &ClientSession::onReadyRead);
        connect(m_socket, &QTcpSocket::disconnected, this, &ClientSession::onDisconnected);
    }

    connect(&m_streamWriter, &PacketStreamWriter::writeToSocket, this, &ClientSession::sendRaw);
}

qint64 ClientSession::sendRaw(const QByteArray& packet)
{
    if (!m_socket) {
        return -1;
    }

    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        return m_socket->write(packet);
    }

    return -1;
}

void ClientSession::enqueueData(CmdType type, const QByteArray& body)
{
    m_streamWriter.enqueue(type, body);
}

QString ClientSession::peerAddress() const
{
    return m_socket ? m_socket->peerAddress().toString() : QString();
}

quint16 ClientSession::peerPort() const
{
    return m_socket ? m_socket->peerPort() : 0;
}

void ClientSession::onReadyRead()
{
    if (!m_socket) {
        return;
    }

    const auto parsed = m_streamParser.appendAndParse(m_socket->readAll());
    for (const auto& result : parsed) {
        if (result.valid) {
            emit commandReceived(result.packet.type, result.packet.body);
        } else {
            emit logMessage(QStringLiteral("收到无效数据包（校验失败）"));
        }
    }
}

void ClientSession::onDisconnected()
{
    emit sessionClosed(this);
}
