#pragma once

#include <QObject>

class RemoteConnection;
class ClientCommandHandler;
class RemoteDesktopWidget;
class QWidget;

class RemoteDesktopController : public QObject
{
    Q_OBJECT
public:
    explicit RemoteDesktopController(RemoteConnection* connection,
        ClientCommandHandler* commandHandler,
        QWidget* parent = nullptr);

    void show();

private:
    void setupConnections();

private:
    RemoteConnection* m_connection = nullptr;
    ClientCommandHandler* m_commandHandler = nullptr;
    RemoteDesktopWidget* m_widget = nullptr;
};
