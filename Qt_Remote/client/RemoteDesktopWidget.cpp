#include "RemoteDesktopWidget.h"

#include <QPainter>
#include <QTimer>

#include "RemoteConnection.h"

RemoteDesktopWidget::RemoteDesktopWidget(QWidget* parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setAutoFillBackground(true);

    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);

    m_mouseTimer.start();

    m_lockButton = new QPushButton(QStringLiteral("锁机"), this);
    m_unlockButton = new QPushButton(QStringLiteral("解锁"), this);

    connect(m_lockButton, &QPushButton::clicked, this, &RemoteDesktopWidget::sendLockMachineCommand);
    connect(m_unlockButton, &QPushButton::clicked, this, &RemoteDesktopWidget::sendUnlockMachineCommand);
}

void RemoteDesktopWidget::setConnection(RemoteConnection* conn)
{
    m_connection = conn;
}

void RemoteDesktopWidget::onScreenDataReceived(const QPixmap& pixmap)
{
    m_currentFrame = pixmap;
    update();

    if (m_connection && this->isVisible()) {
        QTimer::singleShot(30, this, [this]() {
            if (m_connection && this->isVisible()) {
                m_connection->sendPacket(CmdType::ScreenData);
            }
        });
    }
}

void RemoteDesktopWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    const QRect displayRect = remoteDisplayRect();
    painter.fillRect(displayRect, Qt::black);

    if (!m_currentFrame.isNull()) {
        painter.drawPixmap(displayRect, m_currentFrame);
    }
}

void RemoteDesktopWidget::resizeEvent(QResizeEvent* event)
{
    const int areaHeight = 56;
    const int buttonW = 96;
    const int buttonH = 32;
    const int spacing = 12;
    const int left = 12;
    const int top = (areaHeight - buttonH) / 2;

    m_lockButton->setGeometry(left, top, buttonW, buttonH);
    m_unlockButton->setGeometry(left + buttonW + spacing, top, buttonW, buttonH);

    QWidget::resizeEvent(event);
}

void RemoteDesktopWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (!isInRemoteDisplay(event->pos())) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    if (m_mouseTimer.elapsed() > 30) {
        sendMouseEvent(MouseEventType::Move, event->pos());
        m_mouseTimer.restart();
    }
    QWidget::mouseMoveEvent(event);
}

void RemoteDesktopWidget::closeEvent(QCloseEvent* event)
{
    m_currentFrame = QPixmap();
    QWidget::closeEvent(event);
}

void RemoteDesktopWidget::mousePressEvent(QMouseEvent* event)
{
    if (!isInRemoteDisplay(event->pos())) {
        QWidget::mousePressEvent(event);
        return;
    }

    MouseEventType type = MouseEventType::None;
    switch (event->button()) {
    case Qt::LeftButton:
        type = MouseEventType::LeftPress;
        break;
    case Qt::RightButton:
        type = MouseEventType::RightPress;
        break;
    case Qt::MiddleButton:
        type = MouseEventType::MiddlePress;
        break;
    default:
        break;
    }

    if (type != MouseEventType::None) {
        sendMouseEvent(type, event->pos());
    }

    QWidget::mousePressEvent(event);
}

void RemoteDesktopWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (!isInRemoteDisplay(event->pos())) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    MouseEventType type = MouseEventType::None;
    switch (event->button()) {
    case Qt::LeftButton:
        type = MouseEventType::LeftRelease;
        break;
    case Qt::RightButton:
        type = MouseEventType::RightRelease;
        break;
    case Qt::MiddleButton:
        type = MouseEventType::MiddleRelease;
        break;
    default:
        break;
    }

    if (type != MouseEventType::None) {
        sendMouseEvent(type, event->pos());
    }

    QWidget::mouseReleaseEvent(event);
}

void RemoteDesktopWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (!isInRemoteDisplay(event->pos())) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    MouseEventType type = MouseEventType::None;
    switch (event->button()) {
    case Qt::LeftButton:
        type = MouseEventType::LeftDoubleClick;
        break;
    case Qt::RightButton:
        type = MouseEventType::RightDoubleClick;
        break;
    case Qt::MiddleButton:
        type = MouseEventType::MiddleDoubleClick;
        break;
    default:
        break;
    }

    if (type != MouseEventType::None) {
        sendMouseEvent(type, event->pos());
    }

    QWidget::mouseDoubleClickEvent(event);
}

void RemoteDesktopWidget::wheelEvent(QWheelEvent* event)
{
    if (!isInRemoteDisplay(event->position().toPoint())) {
        QWidget::wheelEvent(event);
        return;
    }

    sendMouseEvent(MouseEventType::Scroll, event->position().toPoint(), event->angleDelta().y());
    QWidget::wheelEvent(event);
}

void RemoteDesktopWidget::sendMouseEvent(MouseEventType type, const QPoint& localPos, int scrollDelta)
{
    if (!m_connection || m_currentFrame.isNull()) {
        return;
    }

    const QRect displayRect = remoteDisplayRect();
    const int localW = displayRect.width();
    const int localH = displayRect.height();
    const int remoteW = m_currentFrame.width();
    const int remoteH = m_currentFrame.height();

    if (localW <= 0 || localH <= 0 || remoteW <= 0 || remoteH <= 0) {
        return;
    }

    const QPoint displayPos = localPos - displayRect.topLeft();
    const int mappedX = qBound(0, displayPos.x() * remoteW / localW, remoteW - 1);
    const int mappedY = qBound(0, displayPos.y() * remoteH / localH, remoteH - 1);

    MouseEvent mouseEvent;
    mouseEvent.eventType = type;
    mouseEvent.x = mappedX;
    mouseEvent.y = mappedY;
    mouseEvent.scrollDelta = scrollDelta;

    QByteArray body;
    body.append(reinterpret_cast<const char*>(&mouseEvent), sizeof(MouseEvent));
    m_connection->sendPacket(CmdType::MouseInput, body);
}

QRect RemoteDesktopWidget::remoteDisplayRect() const
{
    static constexpr int kToolBarHeight = 56;
    const int top = qMin(height(), kToolBarHeight);
    return QRect(0, top, width(), qMax(0, height() - top));
}

bool RemoteDesktopWidget::isInRemoteDisplay(const QPoint& pos) const
{
    return remoteDisplayRect().contains(pos);
}

//TODO:解锁和锁机需要互斥使用，后续可以改成一个按钮，点击后根据当前状态发送锁机或解锁命令
void RemoteDesktopWidget::sendLockMachineCommand()
{
    if (m_connection) {
        m_connection->sendPacket(CmdType::LockMachine);
    }
}

void RemoteDesktopWidget::sendUnlockMachineCommand()
{
    if (m_connection) {
        m_connection->sendPacket(CmdType::UnLockMachine);
    }
}
