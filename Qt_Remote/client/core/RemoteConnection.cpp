#include "RemoteConnection.h"

#include <QMetaObject>
#include <QThread>
#include <QUuid>
#include <cstring>

RemoteConnection::RemoteConnection(QObject* parent)
    : QObject(parent)
    , m_machineId(QUuid::createUuid().toString(QUuid::WithoutBraces))
    , m_mainSocket(new QTcpSocket(this))
    , m_fileSocket(new QTcpSocket(this))
{
    connect(m_mainSocket, &QTcpSocket::connected, this, &RemoteConnection::onMainConnected);
    connect(m_mainSocket, &QTcpSocket::disconnected, this, &RemoteConnection::disconnected);
    connect(m_mainSocket, &QTcpSocket::readyRead, this, &RemoteConnection::onMainReadyRead);
    connect(m_mainSocket, &QTcpSocket::errorOccurred, this, &RemoteConnection::onMainSocketError);

    connect(m_fileSocket, &QTcpSocket::connected, this, &RemoteConnection::onFileConnected);
    connect(m_fileSocket, &QTcpSocket::readyRead, this, &RemoteConnection::onFileReadyRead);
    connect(m_fileSocket, &QTcpSocket::errorOccurred, this, &RemoteConnection::onFileSocketError);
}

void RemoteConnection::connectToServer(const QString& ip, quint16 port)
{
    if (m_mainSocket->state() == QAbstractSocket::ConnectedState
        || m_fileSocket->state() == QAbstractSocket::ConnectedState) {
        emit logMessage(QStringLiteral("已处于连接状态，无需重复连接"));
        return;
    }
    if (m_mainSocket->state() == QAbstractSocket::ConnectingState
        || m_fileSocket->state() == QAbstractSocket::ConnectingState) {
        emit logMessage(QStringLiteral("正在连接中，请稍候..."));
        return;
    }

    m_mainSocket->setProxy(QNetworkProxy::NoProxy);
    m_fileSocket->setProxy(QNetworkProxy::NoProxy);

    emit logMessage(QString("正在连接到 %1:%2 ...").arg(ip).arg(port));
    m_mainSocket->connectToHost(ip, port);
    m_fileSocket->connectToHost(ip, port);
}

void RemoteConnection::disconnectFromServer()
{
    const bool hasConnectedSocket =
        m_mainSocket->state() == QAbstractSocket::ConnectedState
        || m_fileSocket->state() == QAbstractSocket::ConnectedState;
    if (hasConnectedSocket) {
        emit logMessage(QStringLiteral("正在断开连接..."));
        if (m_mainSocket->state() == QAbstractSocket::ConnectedState) {
            m_mainSocket->disconnectFromHost();
        }
        if (m_fileSocket->state() == QAbstractSocket::ConnectedState) {
            m_fileSocket->disconnectFromHost();
        }
        return;
    }
    emit logMessage(QStringLiteral("当前未连接，无需断开"));
}

bool RemoteConnection::isConnected() const
{
    return m_mainSocket->state() == QAbstractSocket::ConnectedState;
}

void RemoteConnection::sendPacket(CmdType type, const QByteArray& body)
{
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, [this, type, body]() { sendPacket(type, body); }, Qt::QueuedConnection);
        return;
    }

    QTcpSocket* targetSocket = (type == CmdType::DownLoadFile) ? m_fileSocket : m_mainSocket;
    if (!targetSocket || targetSocket->state() != QAbstractSocket::ConnectedState) {
        emit logMessage("【错误】未连接到被控端，无法发送指令！");
        return;
    }

    const QByteArray packet = NetworkPacket::pack(type, body);
    targetSocket->write(packet);
}

void RemoteConnection::onMainConnected()
{
    emit connected();
    sendAuthPacket(m_mainSocket, SocketRole::Main);
}

void RemoteConnection::onFileConnected()
{
    sendAuthPacket(m_fileSocket, SocketRole::FileTransfer);
}

void RemoteConnection::onMainReadyRead()
{
    processIncomingData(m_mainSocket, m_mainParser);
}

void RemoteConnection::onFileReadyRead()
{
    processIncomingData(m_fileSocket, m_fileParser);
}

void RemoteConnection::onMainSocketError(QAbstractSocket::SocketError /*socketError*/)
{
    emit logMessage(QStringLiteral("主通道 Socket错误：%1").arg(m_mainSocket->errorString()));
    emit errorOccurred(m_mainSocket->errorString());
}

void RemoteConnection::onFileSocketError(QAbstractSocket::SocketError /*socketError*/)
{
    emit logMessage(QStringLiteral("文件通道 Socket错误：%1").arg(m_fileSocket->errorString()));
}

void RemoteConnection::sendAuthPacket(QTcpSocket* socket, SocketRole role)
{
    if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    AuthEvent authEvent;
    std::strncpy(authEvent.machineId, m_machineId.toUtf8().constData(), sizeof(authEvent.machineId) - 1);
    authEvent.role = role;

    QByteArray body(reinterpret_cast<const char*>(&authEvent), sizeof(AuthEvent));
    socket->write(NetworkPacket::pack(CmdType::AuthConnection, body));
}

void RemoteConnection::processIncomingData(QTcpSocket* socket, PacketStreamParser& parser)
{
    const auto parsed = parser.appendAndParse(socket->readAll());
    for (const auto& result : parsed) {
        if (result.valid) {
            emit commandReceived(result.packet.type, result.packet.body);
        } else {
            emit logMessage("【警告】收到无效数据包（校验失败），已丢弃");
        }
    }
}
