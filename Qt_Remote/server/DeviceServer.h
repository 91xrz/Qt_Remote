#pragma once

#include <QObject>
#include <QTcpServer>
#include <QList>

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

signals:
    void logMessage(const QString& msg);
    void clientConnected(const QString& ip, quint16 port);
    void clientDisconnected(const QString& ip, quint16 port);
    void statusChanged(const QString& status);

private slots:
    void onNewConnection();

private:
    QTcpServer* m_server = nullptr;
    QList<ClientSession*> m_sessions;
};
