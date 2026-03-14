#include "DeviceServer.h"

DeviceServer::DeviceServer(QObject* parent) : QObject(parent)
{
	m_server = new QTcpServer(this);// 创建 TCP 服务器对象 析构时会自动close()  可能涉及到手动close
    m_cmdHandler = new CommandHandler(this);
    connect(m_server, &QTcpServer::newConnection, this, &DeviceServer::onNewConnection);

    //本地测试业务
    connect(m_cmdHandler, &CommandHandler::logMessage, this, &DeviceServer::logMessage);
}

bool DeviceServer::startListen(int port)
{
    if (!m_server->listen(QHostAddress::Any, port)) {
        emit logMessage("监听失败: " + m_server->errorString());
        return false;
    }
    emit logMessage("服务启动成功，端口: " + QString::number(port));
    return true;
}

void DeviceServer::stopListen()
{
    if (!m_server->isListening()) {
        emit logMessage("服务未在监听状态");
        return;
    }

    m_server->close();
    emit logMessage("服务已停止监听");
}

void DeviceServer::testFunction()
{
    if (m_cmdHandler)
        m_cmdHandler->MakeDriverInfo();
}

void DeviceServer::onNewConnection()
{       // 1. 获取新连接
    QTcpSocket* socket = m_server->nextPendingConnection();
    if (!socket) return;

    QString ip = socket->peerAddress().toString();

    // 2. 【关键】创建 Session 并移交 Socket (过继孩子)
    ClientSession* session = new ClientSession(socket, this);

    // 3. 保存 Session 指针 (防止内存泄漏或丢失控制权)
    m_sessions.append(session);

    //连接业务层和当前被控端 相应的信号进行处理
    connect(session, &ClientSession::ReceiveCommand,
        m_cmdHandler, &CommandHandler::onHandlerCommand);

    connect(m_cmdHandler, &CommandHandler::sendPacket,
        session, &ClientSession::sendRaw);


    // 比如：如果 Session 说“我结束了”，Server 就把它从列表里删掉
    connect(session, &ClientSession::sessionClosed, this, [=](ClientSession* s) {
        m_sessions.removeOne(s);
        s->deleteLater(); // 销毁对象
        qDebug() << "会话已销毁，当前在线人数：" << m_sessions.size();
        });

    // 5. 通知界面
    emit clientConnected(ip, socket->peerPort());

}
