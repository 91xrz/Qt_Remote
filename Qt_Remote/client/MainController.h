#pragma once

#include <QObject>

class Qt_Remote;
class RemoteConnection;
class ClientCommandHandler;
class FileManagerController;
class RemoteDesktopController;

class MainController : public QObject
{
    Q_OBJECT
public:
    explicit MainController(QObject* parent = nullptr);
    void show();

private:
    Qt_Remote* m_mainWindow = nullptr;
    RemoteConnection* m_connection = nullptr;
    ClientCommandHandler* m_commandHandler = nullptr;
    FileManagerController* m_fileManagerController = nullptr;
    RemoteDesktopController* m_remoteDesktopController = nullptr;
};
