#pragma once

#include <QObject>
#include <QTcpSocket>
#include "NetworkData.h"
#include "PacketStreamParser.h"

class ClientSession : public QObject
{
    Q_OBJECT
public:
    explicit ClientSession(QTcpSocket* mainSocket, QObject* parent = nullptr);
    void bindFileSocket(QTcpSocket* fileSocket);
    qint64 sendRaw(const QByteArray& packet);
    QString peerAddress() const;
    quint16 peerPort() const;
    bool hasFileSocket() const;

public slots:
    void sendPacket(CmdType type, const QByteArray& body);

signals:
    void commandReceived(CmdType cmdType, const QByteArray& data);
    void sessionClosed(ClientSession* client);
    void logMessage(const QString& msg);

private slots:
    void onMainReadyRead();
    void onFileReadyRead();
    void onMainDisconnected();
    void onFileDisconnected();

private:
    bool isFileChannelCommand(CmdType type) const;

    QTcpSocket* m_mainSocket = nullptr;
    QTcpSocket* m_fileSocket = nullptr;
    PacketStreamParser m_mainStreamParser;
    PacketStreamParser m_fileStreamParser;
};
