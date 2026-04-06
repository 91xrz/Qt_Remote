#include "DeviceServer.h"

#include <QTcpSocket>
#include "ClientSession.h"

DeviceServer::DeviceServer(QObject* parent)
    : QObject(parent), m_server(new QTcpServer(this)),m_cmdHandler(new CommandHandler(this))
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

        //业务分发
        connect(session, &ClientSession::commandReceived,
            m_cmdHandler, &CommandHandler::onHandlerCommand);
        connect(m_cmdHandler, &CommandHandler::sendPacket,
            session, &ClientSession::sendRaw);


        connect(session, &ClientSession::sessionClosed, this, [this](ClientSession* s) {
            const QString ip = s->peerAddress();
            const quint16 port = s->peerPort();
            m_sessions.removeOne(s);
            emit clientDisconnected(ip, port);
            emit logMessage(QStringLiteral("客户端断开: %1:%2，当前在线: %3")
                .arg(ip)
                .arg(port)
                .arg(m_sessions.size()));
            s->deleteLater();
        });

        emit clientConnected(session->peerAddress(), session->peerPort());
        emit logMessage(QStringLiteral("客户端连接: %1:%2，当前在线: %3")
            .arg(session->peerAddress())
            .arg(session->peerPort())
            .arg(m_sessions.size()));
    }
}
