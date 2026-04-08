#include "RemoteDesktopWidget.h"

#include <QPainter>

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
}

void RemoteDesktopWidget::setConnection(RemoteConnection* conn)
{
    m_connection = conn;
}

void RemoteDesktopWidget::onScreenDataReceived(const QPixmap& pixmap)
{
    m_currentFrame = pixmap;
    update();

    if (m_connection) {
        m_connection->sendPacket(CmdType::ScreenData);
    }
}

void RemoteDesktopWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (!m_currentFrame.isNull()) {
        painter.drawPixmap(rect(), m_currentFrame);
    }
}

void RemoteDesktopWidget::mouseMoveEvent(QMouseEvent* event)
{
    sendMouseEvent(MouseEventType::Move, event->pos());
    QWidget::mouseMoveEvent(event);
}

void RemoteDesktopWidget::mousePressEvent(QMouseEvent* event)
{
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
    sendMouseEvent(MouseEventType::Scroll, event->position().toPoint(), event->angleDelta().y());
    QWidget::wheelEvent(event);
}

void RemoteDesktopWidget::sendMouseEvent(MouseEventType type, const QPoint& localPos, int scrollDelta)
{
    if (!m_connection || m_currentFrame.isNull()) {
        return;
    }

    const int localW = width();
    const int localH = height();
    const int remoteW = m_currentFrame.width();
    const int remoteH = m_currentFrame.height();

    if (localW <= 0 || localH <= 0 || remoteW <= 0 || remoteH <= 0) {
        return;
    }

    const int mappedX = qBound(0, localPos.x() * remoteW / localW, remoteW - 1);
    const int mappedY = qBound(0, localPos.y() * remoteH / localH, remoteH - 1);

    MouseEvent mouseEvent;
    mouseEvent.eventType = type;
    mouseEvent.x = mappedX;
    mouseEvent.y = mappedY;
    mouseEvent.scrollDelta = scrollDelta;

    QByteArray body;
    body.append(reinterpret_cast<const char*>(&mouseEvent), sizeof(MouseEvent));
    m_connection->sendPacket(CmdType::MouseInput, body);
}
