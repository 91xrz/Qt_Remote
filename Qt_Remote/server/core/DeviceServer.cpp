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

        auto* session = new ClientSession(socket, this);
        m_sessions.append(session);

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

        emit clientConnected(session->peerAddress(), session->peerPort());
        emit onlineCountChanged(m_sessions.size());
        emit logMessage(QStringLiteral("客户端连接: %1:%2，当前在线: %3")
            .arg(session->peerAddress())
            .arg(session->peerPort())
            .arg(m_sessions.size()));
    }
}

void DeviceServer::onCommandFromSession(ClientSession* session, CmdType type, const QByteArray& body)
{
    m_activeSession = session;
    emit commandReceived(type, body);
}
