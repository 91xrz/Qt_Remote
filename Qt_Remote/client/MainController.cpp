#include "MainController.h"

#include "ClientCommandHandler.h"
#include "FileManagerController.h"
#include "Qt_Remote.h"
#include "RemoteConnection.h"
#include "RemoteDesktopController.h"

MainController::MainController(QObject* parent)
    : QObject(parent)
{
    m_connection = new RemoteConnection(this);
    m_commandHandler = new ClientCommandHandler(this);
    m_mainWindow = new Qt_Remote();

    connect(m_connection, &RemoteConnection::commandReceived,
        m_commandHandler, &ClientCommandHandler::onCommandReceived);

    connect(m_commandHandler, &ClientCommandHandler::sigLogMessage,
        m_mainWindow, &Qt_Remote::appendLogMessage);
    connect(m_connection, &RemoteConnection::logMessage,
        m_mainWindow, &Qt_Remote::appendLogMessage);

    connect(m_connection, &RemoteConnection::connected,
        m_mainWindow, &Qt_Remote::onConnected);
    connect(m_connection, &RemoteConnection::disconnected,
        m_mainWindow, &Qt_Remote::onDisconnected);
    connect(m_connection, &RemoteConnection::errorOccurred,
        m_mainWindow, &Qt_Remote::onConnectionError);

    connect(m_mainWindow, &Qt_Remote::sigConnectRequested, this,
        [this](const QString& ip, quint16 port) {
            m_mainWindow->setConnectingState(ip, port);
            m_connection->connectToServer(ip, port);
        });

    connect(m_mainWindow, &Qt_Remote::sigDisconnectRequested, this,
        [this]() {
            m_mainWindow->appendLogMessage(QStringLiteral("请求断开连接"));
            m_connection->disconnectFromServer();
        });

    connect(m_mainWindow, &Qt_Remote::sigOpenFileManagerRequested, this,
        [this]() {
            if (!m_connection->isConnected()) {
                m_mainWindow->appendLogMessage(QStringLiteral("【文件管理】当前未连接，无法启动文件管理器"));
                return;
            }

            if (!m_fileManagerController) {
                m_fileManagerController = new FileManagerController(m_connection, m_commandHandler, m_mainWindow);
            }
            m_fileManagerController->show();
        });

    connect(m_mainWindow, &Qt_Remote::sigOpenRemoteDesktopRequested, this,
        [this]() {
            if (!m_connection->isConnected()) {
                m_mainWindow->appendLogMessage(QStringLiteral("【远程桌面】当前未连接，无法启动屏幕监控"));
                return;
            }

            if (!m_remoteDesktopController) {
                m_remoteDesktopController = new RemoteDesktopController(m_connection, m_commandHandler, m_mainWindow);
            }
            m_remoteDesktopController->show();
        });

    connect(m_mainWindow, &Qt_Remote::sigTestRequested, this,
        [this]() {
            if (!m_connection->isConnected()) {
                m_mainWindow->appendLogMessage(QStringLiteral("【功能测试】当前未连接，无法发送测试指令"));
                return;
            }
            m_mainWindow->appendLogMessage(QStringLiteral("【功能测试】准备发送测试指令（DriverInfo）"));
            m_connection->sendPacket(CmdType::DriverInfo);
        });
}

void MainController::show()
{
    m_mainWindow->show();
}
