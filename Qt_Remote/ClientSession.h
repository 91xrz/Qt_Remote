#pragma once
#include <QObject>
#include <QTcpSocket>
#include "NetworkData.h"
class ClientSession : public QObject
{
    Q_OBJECT
public:
    // 构造函数接收那个刚连上的 socket
    explicit ClientSession(QTcpSocket* socket, QObject* parent = nullptr);
    qint64 sendRaw(const QByteArray& packet);
signals:
    // 收到指令了（比如对方想点击鼠标），发信号告诉逻辑层去执行
    void commandReceived(QString cmdType, QByteArray data);

    // 掉线了，通知 Server 把我从列表里删掉
    void sessionClosed(ClientSession* client);

public slots:
    // 发送数据的接口（比如把屏幕截图发给对方）
    void sendScreenData(const QByteArray& imgData);

private slots:
    void onReadyRead();    // 处理收到的数据
    void onDisconnected(); // 处理断开

private:
    QTcpSocket* m_socket;  // 持有 socket 指针
	QByteArray m_buffer;   // 处理粘包的缓冲区
};