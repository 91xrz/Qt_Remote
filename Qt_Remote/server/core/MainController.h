#pragma once

#include <QObject>

class ServerWindow;
class LockScreenWidget;
class DeviceServer;
class CommandHandler;

class MainController : public QObject
{
    Q_OBJECT
public:
    explicit MainController(QObject* parent = nullptr);
    void show();

private:
    ServerWindow* m_serverWindow = nullptr;
    LockScreenWidget* m_lockScreenWidget = nullptr;
    DeviceServer* m_deviceServer = nullptr;
    CommandHandler* m_commandHandler = nullptr;
};
