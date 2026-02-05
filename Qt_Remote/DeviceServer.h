#pragma once
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include "ClientSession.h"



#pragma execution_character_set("utf-8")
class DeviceServer : public QObject
{
    Q_OBJECT
public:
    explicit DeviceServer(QObject* parent = nullptr);

	QList<ClientSession*> m_sessions;// 保存所有的客户端连接
    // 对外提供的接口：启动服务
    bool startListen(int port);
    // 对外提供的接口：关闭服务
    void stopListen();

signals:
    // 信号：通知 UI 更新状态（比如“有人连接了”）
    // 注意：只发信号，绝对不要在 cpp 里写 ui->xxx
    void logMessage(QString msg);
    void clientConnected(QString ip, int port);

private slots:
    void onNewConnection(); // 内部处理连接

private:
    QTcpServer* m_server;
};

