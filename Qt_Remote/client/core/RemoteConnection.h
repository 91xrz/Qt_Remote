#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QNetworkProxy>
#include "NetworkData.h"
#include "PacketStreamParser.h"

class RemoteConnection : public QObject
{
    Q_OBJECT
public:
    explicit RemoteConnection(QObject* parent = nullptr);

    // 主动连接被控端
    void connectToServer(const QString& ip, quint16 port);
    // 断开连接
    void disconnectFromServer();
    // 是否在线
    bool isConnected() const;

    // 核心发包函数 (UI层直接调这个)
    void sendPacket(CmdType type, const QByteArray& body = QByteArray());

signals:
    // 网络状态信号，供 UI 界面更新
    void connected();
    void disconnected();
    void errorOccurred(const QString& errorMsg);
    void logMessage(const QString& msg);

    // 核心收包信号 (把解析好的指令丢给 UI 去处理)
    void commandReceived(CmdType type, const QByteArray& data);

private slots:
    void onMainSocketConnected();
    void onFileSocketConnected();
    void onSocketDisconnected();
    void onMainSocketReadyRead();
    void onFileSocketReadyRead();
    void onSocketError(QAbstractSocket::SocketError socketError);

private:
    bool isFileChannelCommand(CmdType type) const;
    void emitConnectedIfReady();

    QTcpSocket* m_mainSocket = nullptr;
    QTcpSocket* m_fileSocket = nullptr;
    PacketStreamParser m_mainStreamParser; // 主通道粘包解析器
    PacketStreamParser m_fileStreamParser; // 文件通道粘包解析器
};
