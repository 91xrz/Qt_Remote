#include "RemoteConnection.h"

#include <QMetaObject>
#include <QThread>

RemoteConnection::RemoteConnection(QObject* parent)
    : QObject(parent),
      m_mainSocket(new QTcpSocket(this)),
      m_fileSocket(new QTcpSocket(this))
{
    // 主通道信号
    connect(m_mainSocket, &QTcpSocket::connected, this, &RemoteConnection::onMainSocketConnected);
    connect(m_mainSocket, &QTcpSocket::disconnected, this, &RemoteConnection::onSocketDisconnected);
    connect(m_mainSocket, &QTcpSocket::readyRead, this, &RemoteConnection::onMainSocketReadyRead);
    connect(m_mainSocket, &QTcpSocket::errorOccurred, this, &RemoteConnection::onSocketError);

    // 文件通道信号
    connect(m_fileSocket, &QTcpSocket::connected, this, &RemoteConnection::onFileSocketConnected);
    connect(m_fileSocket, &QTcpSocket::disconnected, this, &RemoteConnection::onSocketDisconnected);
    connect(m_fileSocket, &QTcpSocket::readyRead, this, &RemoteConnection::onFileSocketReadyRead);
    connect(m_fileSocket, &QTcpSocket::errorOccurred, this, &RemoteConnection::onSocketError);
}

void RemoteConnection::connectToServer(const QString& ip, quint16 port)
{
    if (isConnected()) {
        emit logMessage(QStringLiteral("已处于连接状态，无需重复连接"));
        return;
    }
    if (m_mainSocket->state() == QAbstractSocket::ConnectingState ||
        m_fileSocket->state() == QAbstractSocket::ConnectingState) {
        emit logMessage(QStringLiteral("正在连接中，请稍候..."));
        return;
    }

    m_mainSocket->setProxy(QNetworkProxy::NoProxy);
    m_fileSocket->setProxy(QNetworkProxy::NoProxy);

    emit logMessage(QString("正在连接到 %1:%2 ...").arg(ip).arg(port));
    // 单端口双 Socket：同一端口建立主通道与文件通道
    m_mainSocket->connectToHost(ip, port);
    m_fileSocket->connectToHost(ip, port);
}

void RemoteConnection::disconnectFromServer()
{
    if (m_mainSocket->state() == QAbstractSocket::ConnectedState ||
        m_fileSocket->state() == QAbstractSocket::ConnectedState) {
        emit logMessage(QStringLiteral("正在断开连接..."));
        m_mainSocket->disconnectFromHost();
        m_fileSocket->disconnectFromHost();
        return;
    }
    emit logMessage(QStringLiteral("当前未连接，无需断开"));
}

bool RemoteConnection::isConnected() const
{
    return m_mainSocket->state() == QAbstractSocket::ConnectedState &&
        m_fileSocket->state() == QAbstractSocket::ConnectedState;
}

void RemoteConnection::sendPacket(CmdType type, const QByteArray& body)
{
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, [this, type, body]() { sendPacket(type, body); }, Qt::QueuedConnection);
        return;
    }

    // 中文注释：根据命令类型进行路由，文件类命令走文件通道，其余走主通道
    QTcpSocket* targetSocket = isFileChannelCommand(type) ? m_fileSocket : m_mainSocket;
    if (!targetSocket || targetSocket->state() != QAbstractSocket::ConnectedState) {
        emit logMessage("【错误】未连接到被控端，无法发送指令！");
        return;
    }

    // 复用 common 里的 NetworkPacket 打包工具
    const QByteArray packet = NetworkPacket::pack(type, body);
    targetSocket->write(packet);
}

void RemoteConnection::onMainSocketConnected()
{
    // 主通道握手认证
    m_mainSocket->write(NetworkPacket::pack(CmdType::AuthMainChannel, QByteArray()));
    emit logMessage(QStringLiteral("主通道连接成功，已发送 AuthMainChannel"));
    emitConnectedIfReady();
}

void RemoteConnection::onFileSocketConnected()
{
    // 文件通道握手认证
    m_fileSocket->write(NetworkPacket::pack(CmdType::AuthFileChannel, QByteArray()));
    emit logMessage(QStringLiteral("文件通道连接成功，已发送 AuthFileChannel"));
    emitConnectedIfReady();
}

void RemoteConnection::onSocketDisconnected()
{
    emit disconnected();
}

void RemoteConnection::onMainSocketReadyRead()
{
    // 中文注释：主通道异步读取，复用粘包解析器，避免半包/粘包导致协议错乱
    const auto parsed = m_mainStreamParser.appendAndParse(m_mainSocket->readAll());
    for (const auto& result : parsed) {
        if (result.valid) {
            // 主通道接收到的业务包继续交由上层 UI 处理
            emit commandReceived(result.packet.type, result.packet.body);
        }
        else {
            emit logMessage("【警告】收到无效数据包（校验失败），已丢弃");
        }
    }
}

void RemoteConnection::onFileSocketReadyRead()
{
    // 中文注释：文件通道异步读取，同样复用粘包解析器，确保大文件下载包边界正确
    const auto parsed = m_fileStreamParser.appendAndParse(m_fileSocket->readAll());
    for (const auto& result : parsed) {
        if (result.valid) {
            emit commandReceived(result.packet.type, result.packet.body);
        } else {
            emit logMessage("【警告】文件通道收到无效数据包（校验失败），已丢弃");
        }
    }
}

void RemoteConnection::onSocketError(QAbstractSocket::SocketError /*socketError*/)
{
    auto* socket = qobject_cast<QTcpSocket*>(sender());
    const QString channel = (socket == m_fileSocket) ? QStringLiteral("文件通道") : QStringLiteral("主通道");
    const QString error = socket ? socket->errorString() : QStringLiteral("未知错误");
    emit logMessage(QStringLiteral("%1 Socket错误：%2").arg(channel).arg(error));
    emit errorOccurred(error);
}

bool RemoteConnection::isFileChannelCommand(CmdType type) const
{
    return type == CmdType::DownLoadFile ||
        type == CmdType::DirInfo ||
        type == CmdType::DriverInfo;
}

void RemoteConnection::emitConnectedIfReady()
{
    if (isConnected()) {
        emit connected();
    }
}
