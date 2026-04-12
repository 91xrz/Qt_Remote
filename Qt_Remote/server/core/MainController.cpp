#include "core/MainController.h"

#include "controller/CommandHandler.h"
#include "core/DeviceServer.h"
#include "view/LockScreenWidget.h"
#include "view/ServerWindow.h"

MainController::MainController(QObject* parent)
    : QObject(parent)
{
    m_serverWindow = new ServerWindow();
    m_lockScreenWidget = new LockScreenWidget();
    m_deviceServer = new DeviceServer(this);
    m_commandHandler = new CommandHandler(this);

    connect(m_serverWindow, &ServerWindow::sigToggleListenRequested, this,
        [this](quint16 port) {
            if (m_deviceServer->isListening()) {
                m_deviceServer->stopListen();
                m_serverWindow->updateListeningState(false, 0);
                return;
            }

            if (m_deviceServer->startListen(port)) {
                m_serverWindow->updateListeningState(true, m_deviceServer->listeningPort());
            }
        });

    connect(m_deviceServer, &DeviceServer::logMessage,
        m_serverWindow, &ServerWindow::appendLog);
    connect(m_commandHandler, &CommandHandler::logMessage,
        m_serverWindow, &ServerWindow::appendLog);

    connect(m_deviceServer, &DeviceServer::statusChanged,
        m_serverWindow, &ServerWindow::updateStatus);
    connect(m_deviceServer, &DeviceServer::onlineCountChanged,
        m_serverWindow, &ServerWindow::updateOnlineCount);

    connect(m_deviceServer, &DeviceServer::clientConnected, this,
        [this](const QString& ip, quint16 port) {
            m_serverWindow->appendLog(QStringLiteral("连接事件: %1:%2").arg(ip).arg(port));
        });

    connect(m_deviceServer, &DeviceServer::clientDisconnected, this,
        [this](const QString& ip, quint16 port) {
            m_serverWindow->appendLog(QStringLiteral("断开事件: %1:%2").arg(ip).arg(port));
        });

    connect(m_deviceServer, &DeviceServer::commandReceived,
        m_commandHandler, &CommandHandler::onHandlerCommand);
    connect(m_commandHandler, &CommandHandler::sendData,
        m_deviceServer, &DeviceServer::sendToActiveSession);

    connect(m_commandHandler, &CommandHandler::sigLockMachineRequested, this,
        [this]() {
            QByteArray payload;
            if (m_lockScreenWidget->isHidden()) {
                m_lockScreenWidget->lock();
                payload.append(static_cast<char>(LockResult::LockSuccess));
            } else {
                payload.append(static_cast<char>(LockResult::LockFailed));
            }
            m_deviceServer->sendToActiveSession(CmdType::LockMachine, payload);
        });

    connect(m_commandHandler, &CommandHandler::sigUnlockMachineRequested, this,
        [this]() {
            m_lockScreenWidget->unlock();
            m_deviceServer->sendToActiveSession(CmdType::UnLockMachine, QByteArray());
        });

    connect(m_lockScreenWidget, &LockScreenWidget::unlockedLocally, this,
        [this]() {
            m_serverWindow->appendLog(QStringLiteral("【调试】本地用户已通过 Insert 键解锁"));
            m_deviceServer->sendToActiveSession(CmdType::UnLockMachine, QByteArray());
        });

    m_serverWindow->updateListeningState(false, 0);
    m_serverWindow->updateOnlineCount(0);
}

void MainController::show()
{
    m_serverWindow->show();
}
