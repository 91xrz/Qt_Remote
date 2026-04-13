#include "ClientSession.h"

#include <QMetaObject>
#include <QThread>

ClientSession::ClientSession(const QString& machineId, QObject* parent)
    : QObject(parent), m_machineId(machineId)
{
}

QString ClientSession::machineId() const
{
    return m_machineId;
}

void ClientSession::bindMainSocket(QTcpSocket* socket)
{
    if (!socket) {
        return;
    }
    closeAndDeleteSocket(m_mainSocket);
    m_mainSocket = socket;
    m_mainSocket->setParent(this);
    connect(m_mainSocket, &QTcpSocket::readyRead, this, &ClientSession::onMainReadyRead);
    connect(m_mainSocket, &QTcpSocket::disconnected, this, &ClientSession::onMainDisconnected);
}

void ClientSession::bindFileSocket(QTcpSocket* socket)
{
    if (!socket) {
        return;
    }
    closeAndDeleteSocket(m_fileSocket);
    m_fileSocket = socket;
    m_fileSocket->setParent(this);
    connect(m_fileSocket, &QTcpSocket::readyRead, this, &ClientSession::onFileReadyRead);
    connect(m_fileSocket, &QTcpSocket::disconnected, this, &ClientSession::onFileDisconnected);
}

qint64 ClientSession::sendRaw(const QByteArray& packet)
{
    if (!m_mainSocket) {
        return -1;
    }

    if (m_mainSocket->state() == QAbstractSocket::ConnectedState) {
        return m_mainSocket->write(packet);
    }

    return -1;
}

void ClientSession::sendPacket(CmdType type, const QByteArray& body)
{
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, [this, type, body]() { sendPacket(type, body); }, Qt::QueuedConnection);
        return;
    }

    QTcpSocket* targetSocket = isFileCommand(type) ? m_fileSocket : m_mainSocket;
    if (!targetSocket || targetSocket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    const QByteArray packet = NetworkPacket::pack(type, body);
    targetSocket->write(packet);
}

QString ClientSession::peerAddress() const
{
    return m_mainSocket ? m_mainSocket->peerAddress().toString() : QString();
}

quint16 ClientSession::peerPort() const
{
    return m_mainSocket ? m_mainSocket->peerPort() : 0;
}

void ClientSession::onMainReadyRead()
{
    if (!m_mainSocket) {
        return;
    }
    dispatchParsedPackets(m_mainParser, m_mainSocket);
}

void ClientSession::onFileReadyRead()
{
    if (!m_fileSocket) {
        return;
    }
    dispatchParsedPackets(m_fileParser, m_fileSocket);
}

void ClientSession::onMainDisconnected()
{
    closeAndDeleteSocket(m_fileSocket);
    emit sessionClosed(this);
}

void ClientSession::onFileDisconnected()
{
    closeAndDeleteSocket(m_fileSocket);
}

bool ClientSession::isFileCommand(CmdType type)
{
    return type == CmdType::DirInfo
        || type == CmdType::DownLoadFile
        || type == CmdType::RunFile
        || type == CmdType::DeleFile;
}

void ClientSession::closeAndDeleteSocket(QTcpSocket*& socket)
{
    if (!socket) {
        return;
    }
    disconnect(socket, nullptr, this, nullptr);
    socket->close();
    socket->deleteLater();
    socket = nullptr;
}

void ClientSession::dispatchParsedPackets(PacketStreamParser& parser, QTcpSocket* socket)
{
    const auto parsed = parser.appendAndParse(socket->readAll());
    for (const auto& result : parsed) {
        if (result.valid) {
            emit commandReceived(result.packet.type, result.packet.body);
        } else {
            emit logMessage(QStringLiteral("收到无效数据包（校验失败）"));
        }
    }
}
