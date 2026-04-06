#include "RemoteConnection.h"

RemoteConnection::RemoteConnection(QObject* parent)
    : QObject(parent), m_socket(new QTcpSocket(this))
{
    // 绑定 Qt 的 Socket 信号
    connect(m_socket, &QTcpSocket::connected, this, &RemoteConnection::connected);
    connect(m_socket, &QTcpSocket::disconnected, this, &RemoteConnection::disconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &RemoteConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &RemoteConnection::onSocketError);
}

void RemoteConnection::connectToServer(const QString& ip, quint16 port)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        return;
    }
    emit logMessage(QString("正在连接到 %1:%2 ...").arg(ip).arg(port));
    m_socket->connectToHost(ip, port);
}

void RemoteConnection::disconnectFromServer()
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
}

bool RemoteConnection::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void RemoteConnection::sendPacket(CmdType type, const QByteArray& body)
{
    if (!isConnected()) {
        emit logMessage("【错误】未连接到被控端，无法发送指令！");
        return;
    }

    // 复用 common 里的 NetworkPacket 打包工具
    QByteArray packet = NetworkPacket::pack(type, body);
    m_socket->write(packet);
}

void RemoteConnection::onReadyRead()
{
    // 复用 common 里的粘包解析器
    const auto parsed = m_streamParser.appendAndParse(m_socket->readAll());
    for (const auto& result : parsed) {
        if (result.valid) {
            // 解析成功，抛给外层 UI 业务去处理
            emit commandReceived(result.packet.type, result.packet.body);
        }
        else {
            emit logMessage("【警告】收到无效数据包（校验失败），已丢弃");
        }
    }
}

void RemoteConnection::onSocketError(QAbstractSocket::SocketError /*socketError*/)
{
    emit errorOccurred(m_socket->errorString());
}