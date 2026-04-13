#pragma once

#include <QObject>
#include <QTcpSocket>
#include "NetworkData.h"
#include "PacketStreamParser.h"

class ClientSession : public QObject
{
    Q_OBJECT
public:
    explicit ClientSession(const QString& machineId, QObject* parent = nullptr);
    QString machineId() const;
    void bindMainSocket(QTcpSocket* socket);
    void bindFileSocket(QTcpSocket* socket);
    qint64 sendRaw(const QByteArray& packet);
    QString peerAddress() const;
    quint16 peerPort() const;

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
    static bool isFileCommand(CmdType type);
    void closeAndDeleteSocket(QTcpSocket*& socket);
    void dispatchParsedPackets(PacketStreamParser& parser, QTcpSocket* socket);

    QString m_machineId;
    QTcpSocket* m_mainSocket = nullptr;
    QTcpSocket* m_fileSocket = nullptr;
    PacketStreamParser m_mainParser;
    PacketStreamParser m_fileParser;
};
