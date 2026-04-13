#include "core/DeviceServer.h"

#include <QTcpSocket>
#include <cstring>
#include <memory>
#include "core/ClientSession.h"
#include "PacketStreamParser.h"

DeviceServer::DeviceServer(QObject* parent)
    : QObject(parent), m_server(new QTcpServer(this))
{
    connect(m_server, &QTcpServer::newConnection, this, &DeviceServer::onNewConnection);
}

bool DeviceServer::startListen(quint16 port)
{
    if (m_server->isListening()) {
        emit logMessage(QStringLiteral("服务已在监听端口 %1").arg(m_server->serverPort()));
        return true;
    }

    if (!m_server->listen(QHostAddress::Any, port)) {
        emit statusChanged(QStringLiteral("监听失败"));
        emit logMessage(QStringLiteral("监听失败: %1").arg(m_server->errorString()));
        return false;
    }

    emit statusChanged(QStringLiteral("监听中"));
    emit logMessage(QStringLiteral("服务启动成功，监听端口: %1").arg(m_server->serverPort()));
    return true;
}

void DeviceServer::stopListen()
{
    if (!m_server->isListening()) {
        emit logMessage(QStringLiteral("服务未在监听状态"));
        return;
    }

    m_server->close();
    emit statusChanged(QStringLiteral("未监听"));
    emit logMessage(QStringLiteral("服务已停止监听"));
}

bool DeviceServer::isListening() const
{
    return m_server->isListening();
}

quint16 DeviceServer::listeningPort() const
{
    return m_server->serverPort();
}

int DeviceServer::sessionCount() const
{
    return m_sessions.size();
}

void DeviceServer::sendToActiveSession(CmdType type, const QByteArray& body)
{
    if (!m_activeSession) {
        emit logMessage(QStringLiteral("无可用会话，忽略发送: %1").arg(static_cast<int>(type)));
        return;
    }
    m_activeSession->sendPacket(type, body);
}

void DeviceServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket* socket = m_server->nextPendingConnection();
        if (!socket) {
            continue;
        }

        auto parser = std::make_shared<PacketStreamParser>();
        auto readyConn = std::make_shared<QMetaObject::Connection>();

        *readyConn = connect(socket, &QTcpSocket::readyRead, this, [this, socket, parser, readyConn]() {
            const auto parsedPackets = parser->appendAndParse(socket->readAll());
            if (parsedPackets.isEmpty()) {
                return;
            }

            const auto& firstPacket = parsedPackets.first();
            if (!firstPacket.valid || firstPacket.packet.type != CmdType::AuthConnection) {
                emit logMessage(QStringLiteral("拒绝未鉴权连接: %1:%2")
                    .arg(socket->peerAddress().toString())
                    .arg(socket->peerPort()));
                socket->disconnectFromHost();
                return;
            }

            const QByteArray& authBody = firstPacket.packet.body;
            if (authBody.size() < static_cast<int>(sizeof(AuthEvent))) {
                emit logMessage(QStringLiteral("鉴权数据长度错误，断开连接: %1:%2")
                    .arg(socket->peerAddress().toString())
                    .arg(socket->peerPort()));
                socket->disconnectFromHost();
                return;
            }

            AuthEvent authEvent;
            std::memcpy(&authEvent, authBody.constData(), sizeof(AuthEvent));
            const QByteArray machineBytes(authEvent.machineId, strnlen(authEvent.machineId, sizeof(authEvent.machineId)));
            const QString machineId = QString::fromUtf8(machineBytes).trimmed();

            if (machineId.isEmpty()) {
                emit logMessage(QStringLiteral("空 machineId，断开连接: %1:%2")
                    .arg(socket->peerAddress().toString())
                    .arg(socket->peerPort()));
                socket->disconnectFromHost();
                return;
            }

            ClientSession* session = m_sessions.value(machineId, nullptr);
            if (!session) {
                session = new ClientSession(machineId, this);
                m_sessions.insert(machineId, session);

                connect(session, &ClientSession::logMessage, this, &DeviceServer::logMessage);
                connect(session, &ClientSession::commandReceived, this,
                    [this, session](CmdType type, const QByteArray& body) {
                        onCommandFromSession(session, type, body);
                    });
                connect(session, &ClientSession::sessionClosed, this, &DeviceServer::handleSessionClosed);

                emit clientConnected(socket->peerAddress().toString(), socket->peerPort());
                emit onlineCountChanged(m_sessions.size());
                emit logMessage(QStringLiteral("客户端已注册 machineId=%1, 地址=%2:%3，当前在线: %4")
                    .arg(machineId)
                    .arg(socket->peerAddress().toString())
                    .arg(socket->peerPort())
                    .arg(m_sessions.size()));
            }

            if (authEvent.role == SocketRole::Main) {
                session->bindMainSocket(socket);
            } else if (authEvent.role == SocketRole::FileTransfer) {
                session->bindFileSocket(socket);
            } else {
                emit logMessage(QStringLiteral("未知 SocketRole，断开连接: %1:%2")
                    .arg(socket->peerAddress().toString())
                    .arg(socket->peerPort()));
                socket->disconnectFromHost();
                return;
            }

            disconnect(*readyConn);
            emit logMessage(QStringLiteral("鉴权成功 machineId=%1, role=%2")
                .arg(machineId)
                .arg(static_cast<int>(authEvent.role)));
        });
    }
}

void DeviceServer::onCommandFromSession(ClientSession* session, CmdType type, const QByteArray& body)
{
    m_activeSession = session;
    emit commandReceived(type, body);
}

void DeviceServer::handleSessionClosed(ClientSession* session)
{
    const QString ip = session->peerAddress();
    const quint16 port = session->peerPort();
    const QString machineId = session->machineId();
    m_sessions.remove(machineId);
    if (m_activeSession == session) {
        m_activeSession = nullptr;
    }

    emit clientDisconnected(ip, port);
    emit onlineCountChanged(m_sessions.size());
    emit logMessage(QStringLiteral("客户端断开 machineId=%1, 地址=%2:%3，当前在线: %4")
        .arg(machineId)
        .arg(ip)
        .arg(port)
        .arg(m_sessions.size()));
    session->deleteLater();
}
