#pragma once

#include <QObject>
#include <QTcpServer>
#include <QHash>
#include "NetworkData.h"

class ClientSession;

class DeviceServer : public QObject
{
    Q_OBJECT
public:
    explicit DeviceServer(QObject* parent = nullptr);

    bool startListen(quint16 port);
    void stopListen();
    bool isListening() const;
    quint16 listeningPort() const;
    int sessionCount() const;

public slots:
    void sendToActiveSession(CmdType type, const QByteArray& body);

signals:
    void logMessage(const QString& msg);
    void clientConnected(const QString& ip, quint16 port);
    void clientDisconnected(const QString& ip, quint16 port);
    void statusChanged(const QString& status);
    void onlineCountChanged(int count);
    void commandReceived(CmdType type, const QByteArray& body);

private slots:
    void onNewConnection();

private:
    void onCommandFromSession(ClientSession* session, CmdType type, const QByteArray& body);
    void handleSessionClosed(ClientSession* session);

    QTcpServer* m_server = nullptr;
    QHash<QString, ClientSession*> m_sessions;
    ClientSession* m_activeSession = nullptr;
};
