#include "RemoteDesktopController.h"

#include "ClientCommandHandler.h"
#include "core/RemoteConnection.h"
#include "view/RemoteDesktopWidget.h"

RemoteDesktopController::RemoteDesktopController(RemoteConnection* connection,
    ClientCommandHandler* commandHandler,
    QWidget* parent)
    : QObject(parent)
    , m_connection(connection)
    , m_commandHandler(commandHandler)
{
    m_widget = new RemoteDesktopWidget(nullptr);
    m_widget->setAttribute(Qt::WA_DeleteOnClose);
    m_widget->setWindowFlags(Qt::Window);
    m_widget->setWindowTitle(QStringLiteral("远程桌面/屏幕监控"));
    m_widget->resize(1920, 1080);

    setupConnections();
}

void RemoteDesktopController::setupConnections()
{
    connect(m_commandHandler, &ClientCommandHandler::sigScreenDataReceived,
        m_widget, &RemoteDesktopWidget::updateFrame);

    connect(m_widget, &RemoteDesktopWidget::sigRequestNextFrame, this, [this]() {
        m_connection->sendPacket(CmdType::ScreenData);
    });

    connect(m_widget, &RemoteDesktopWidget::sigLockMachineRequested, this, [this]() {
        m_connection->sendPacket(CmdType::LockMachine);
    });

    connect(m_widget, &RemoteDesktopWidget::sigUnlockMachineRequested, this, [this]() {
        m_connection->sendPacket(CmdType::UnLockMachine);
    });

    connect(m_widget, &RemoteDesktopWidget::sigMouseInputCaptured, this,
        [this](MouseEventType eventType, int x, int y, int scrollDelta) {
            MouseEvent mouseEvent;
            mouseEvent.eventType = eventType;
            mouseEvent.x = x;
            mouseEvent.y = y;
            mouseEvent.scrollDelta = scrollDelta;

            QByteArray body;
            body.append(reinterpret_cast<const char*>(&mouseEvent), sizeof(MouseEvent));
            m_connection->sendPacket(CmdType::MouseInput, body);
        });

    connect(m_widget, &RemoteDesktopWidget::sigKeyboardInputCaptured, this,
        [this](KeyEventType eventType, uint32_t vkCode) {
            KeyEvent keyEvent;
            keyEvent.eventType = eventType;
            keyEvent.vkCode = vkCode;

            QByteArray body;
            body.append(reinterpret_cast<const char*>(&keyEvent), sizeof(KeyEvent));
            m_connection->sendPacket(CmdType::KeyboardInput, body);
        });
}

void RemoteDesktopController::show()
{
    m_widget->show();
    m_widget->raise();
    m_widget->activateWindow();

    m_connection->sendPacket(CmdType::ScreenData);
}
