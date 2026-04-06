#pragma once

#include <QObject>
#include <QTcpSocket>
#include "NetworkData.h"
#include "PacketStreamParser.h"

class ClientSession : public QObject
{
    Q_OBJECT
public:
    explicit ClientSession(QTcpSocket* socket, QObject* parent = nullptr);
    qint64 sendRaw(const QByteArray& packet);

    QString peerAddress() const;
    quint16 peerPort() const;

signals:
    void commandReceived(CmdType cmdType, const QByteArray& data);
    void sessionClosed(ClientSession* client);
    void logMessage(const QString& msg);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket* m_socket = nullptr;
    PacketStreamParser m_streamParser;
};
