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
    QByteArray data = m_socket->readAll();
    m_buffer.append(data);

    // 2. 只要水缸里的水够做一个包，就一直做
    while (m_buffer.size() >= NetworkPacket::minSize()) {

        // A. 利用工具类：偷看头部，获取包的长度
        int packageSize = NetworkPacket::totalSize(m_buffer);

        // B. 如果包头损坏（比如没找到魔数），工具类通常会返回 -1 或 0
        if (packageSize == 0) {
            m_buffer.remove(0, 1); // 错位了，丢掉一个字节继续找
            continue;
        }

        // C. 水缸里的水还不够做一个包？那就等下一次 onReadyRead 再说
        if (m_buffer.size() < packageSize) {
            break;
        }

        // D. 水够了！把这个包单独舀出来
        QByteArray onePacketData = m_buffer.mid(0, packageSize);

        // E. 利用工具类：解包、校验
        NetworkPacket::UnpackResult result = NetworkPacket::unpack(onePacketData);

        if (result.isValid) {
            // F. 校验成功，分发业务
           // handlePacket(result.type, result.body);
        }
        else {
            qDebug() << "收到脏数据，校验失败";
        }

        // G. 处理完了，把这部分水倒掉
        m_buffer.remove(0, packageSize);
    }


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