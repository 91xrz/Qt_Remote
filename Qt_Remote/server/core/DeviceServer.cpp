#include "core/DeviceServer.h"

#include <QTcpSocket>
#include "core/ClientSession.h"

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

        // 中文注释：新连接先进入握手阶段，等待首包决定是主通道还是文件通道
        m_pendingParsers.insert(socket, PacketStreamParser());
        connect(socket, &QTcpSocket::readyRead, this, [this, socket]() { onPendingSocketReadyRead(socket); });
        connect(socket, &QTcpSocket::disconnected, this, [this, socket]() { onPendingSocketDisconnected(socket); });
        emit logMessage(QStringLiteral("收到待认证连接: %1:%2")
            .arg(socket->peerAddress().toString())
            .arg(socket->peerPort()));
    }
}

void DeviceServer::onCommandFromSession(ClientSession* session, CmdType type, const QByteArray& body)
{
    m_activeSession = session;
    emit commandReceived(type, body);
}

void DeviceServer::onPendingSocketReadyRead(QTcpSocket* socket)
{
    if (!socket || !m_pendingParsers.contains(socket)) {
        return;
    }

    auto& parser = m_pendingParsers[socket];
    const auto parsed = parser.appendAndParse(socket->readAll());
    for (const auto& result : parsed) {
        if (!result.valid) {
            emit logMessage(QStringLiteral("待认证连接收到无效数据包，已忽略"));
            continue;
        }
        handleHandshakePacket(socket, result.packet.type);
        return;
    }
}

void DeviceServer::onPendingSocketDisconnected(QTcpSocket* socket)
{
    if (!socket) {
        return;
    }
    m_pendingParsers.remove(socket);
}

void DeviceServer::bindSessionSignals(ClientSession* session)
{
    connect(session, &ClientSession::logMessage, this, &DeviceServer::logMessage);
    connect(session, &ClientSession::commandReceived, this,
        [this, session](CmdType type, const QByteArray& body) {
            onCommandFromSession(session, type, body);
        });

    connect(session, &ClientSession::sessionClosed, this, [this](ClientSession* s) {
        const QString ip = s->peerAddress();
        const quint16 port = s->peerPort();
        m_sessions.removeOne(s);
        if (m_activeSession == s) {
            m_activeSession = nullptr;
        }
        emit clientDisconnected(ip, port);
        emit onlineCountChanged(m_sessions.size());
        emit logMessage(QStringLiteral("客户端断开: %1:%2，当前在线: %3")
            .arg(ip)
            .arg(port)
            .arg(m_sessions.size()));
        s->deleteLater();
    });
}

void DeviceServer::handleHandshakePacket(QTcpSocket* socket, CmdType type)
{
    if (!socket) {
        return;
    }

    disconnect(socket, nullptr, this, nullptr);
    m_pendingParsers.remove(socket);

    if (type == CmdType::AuthMainChannel) {
        auto* session = new ClientSession(socket, this);
        bindSessionSignals(session);
        m_sessions.append(session);

        emit clientConnected(session->peerAddress(), session->peerPort());
        emit onlineCountChanged(m_sessions.size());
        emit logMessage(QStringLiteral("主通道认证成功: %1:%2，当前在线: %3")
            .arg(session->peerAddress())
            .arg(session->peerPort())
            .arg(m_sessions.size()));
        return;
    }

    if (type == CmdType::AuthFileChannel) {
        ClientSession* session = findSessionForFileChannel(socket);
        if (!session) {
            emit logMessage(QStringLiteral("文件通道认证失败：未找到匹配主通道，连接已关闭"));
            socket->disconnectFromHost();
            socket->deleteLater();
            return;
        }

        session->bindFileSocket(socket);
        emit logMessage(QStringLiteral("文件通道绑定成功: %1:%2")
            .arg(session->peerAddress())
            .arg(session->peerPort()));
        return;
    }

    emit logMessage(QStringLiteral("未知握手类型(%1)，连接已关闭").arg(static_cast<int>(type)));
    socket->disconnectFromHost();
    socket->deleteLater();
}

ClientSession* DeviceServer::findSessionForFileChannel(QTcpSocket* socket) const
{
    if (!socket) {
        return nullptr;
    }

    const QString peerIp = socket->peerAddress().toString();
    for (ClientSession* session : m_sessions) {
        if (session && session->peerAddress() == peerIp && !session->hasFileSocket()) {
            return session;
        }
    }
    return nullptr;
}
