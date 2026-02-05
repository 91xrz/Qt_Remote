#include "ClientSession.h"

ClientSession::ClientSession(QTcpSocket* socket, QObject* parent)
    : QObject(parent), m_socket(socket)
{
    m_socket = socket;
    // 接管 socket 的父对象，这样 Session 析构时，Socket 也会自动析构
    m_socket->setParent(this);

    // 连接 Socket 的关键信号
    connect(m_socket, &QTcpSocket::readyRead, this, &ClientSession::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientSession::onDisconnected);
}

qint64 ClientSession::sendRaw(const QByteArray& packet) {
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
		return m_socket->write(packet);//       发包send();
    }
    return -1;
}

void ClientSession::onReadyRead() //同DealCommand()可能有参数
{
    // 这里处理“粘包”逻辑和协议解析 
	QByteArray data = m_socket->readAll();  //recv()

	// 处理命令，比如鼠标点击
    // emit commandReceived("MOUSE_CLICK", ...);
}

void ClientSession::sendScreenData(const QByteArray& imgData)
{       // 准备数据
    QByteArray packet;
    // ... 拼装协议头 ...

    // 调用发送
    this->sendRaw(packet);
}

void ClientSession::onDisconnected()
{
    // 告诉 Server：我这边完事了，你可以把我 delete 了
    emit sessionClosed(this);

    // 或者直接自杀（如果你不需要 Server 统计在线人数的话）
    // this->deleteLater(); 
}