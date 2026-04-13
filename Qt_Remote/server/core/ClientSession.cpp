#include "ClientSession.h"

#include <QMetaObject>
#include <QThread>

ClientSession::ClientSession(QTcpSocket* mainSocket, QObject* parent)
    : QObject(parent), m_mainSocket(mainSocket)
{
    if (m_mainSocket) {
        m_mainSocket->setParent(this);
        connect(m_mainSocket, &QTcpSocket::readyRead, this, &ClientSession::onMainReadyRead);
        connect(m_mainSocket, &QTcpSocket::disconnected, this, &ClientSession::onMainDisconnected);
    }
}

void ClientSession::bindFileSocket(QTcpSocket* fileSocket)
{
    if (!fileSocket) {
        return;
    }
    if (m_fileSocket == fileSocket) {
        return;
    }
    if (m_fileSocket) {
        m_fileSocket->disconnectFromHost();
        m_fileSocket->deleteLater();
    }
    m_fileSocket = fileSocket;
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

    // 中文注释：服务端回包按命令类型路由，文件相关命令优先走文件通道
    QTcpSocket* targetSocket = isFileChannelCommand(type) && m_fileSocket
        ? m_fileSocket
        : m_mainSocket;
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

bool ClientSession::hasFileSocket() const
{
    return m_fileSocket && m_fileSocket->state() == QAbstractSocket::ConnectedState;
}

void ClientSession::onMainReadyRead()
{
    if (!m_mainSocket) {
        return;
    }

    // 中文注释：主通道异步读取并做粘包解析，保障控制指令低延迟处理
    const auto parsed = m_mainStreamParser.appendAndParse(m_mainSocket->readAll());
    for (const auto& result : parsed) {
        if (result.valid) {
            if (result.packet.type == CmdType::AuthMainChannel || result.packet.type == CmdType::AuthFileChannel) {
                continue;
            }
            emit commandReceived(result.packet.type, result.packet.body);
        } else {
            emit logMessage(QStringLiteral("收到无效数据包（校验失败）"));
        }
    }
}

void ClientSession::onFileReadyRead()
{
    if (!m_fileSocket) {
        return;
    }

    // 中文注释：文件通道异步读取并做粘包解析，避免大文件流拆包错位
    const auto parsed = m_fileStreamParser.appendAndParse(m_fileSocket->readAll());
    for (const auto& result : parsed) {
        if (result.valid) {
            if (result.packet.type == CmdType::AuthMainChannel || result.packet.type == CmdType::AuthFileChannel) {
                continue;
            }
            emit commandReceived(result.packet.type, result.packet.body);
        } else {
            emit logMessage(QStringLiteral("文件通道收到无效数据包（校验失败）"));
        }
    }
}

void ClientSession::onMainDisconnected()
{
    if (m_fileSocket) {
        m_fileSocket->disconnectFromHost();
    }
    emit sessionClosed(this);
}

void ClientSession::onFileDisconnected()
{
    emit logMessage(QStringLiteral("文件通道断开，后续文件命令将回退到主通道"));
    if (m_fileSocket) {
        m_fileSocket->deleteLater();
        m_fileSocket = nullptr;
    }
}

bool ClientSession::isFileChannelCommand(CmdType type) const
{
    return type == CmdType::DownLoadFile ||
        type == CmdType::DirInfo ||
        type == CmdType::DriverInfo;
}
