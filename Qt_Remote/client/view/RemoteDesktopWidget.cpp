#include "RemoteDesktopWidget.h"

#include <QPainter>
#include <QTimer>

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

    connect(m_lockButton, &QPushButton::clicked, this, [this]() {
        emit sigLockMachineRequested();
    });
    connect(m_unlockButton, &QPushButton::clicked, this, [this]() {
        emit sigUnlockMachineRequested();
    });
}


void RemoteDesktopWidget::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    emit sigRequestNextFrame();
}
void RemoteDesktopWidget::updateFrame(const QPixmap& pixmap)
{
    m_currentFrame = pixmap;
    update();

    if (isVisible()) {
        QTimer::singleShot(1, this, [this]() {
            if (isVisible()) {
                emit sigRequestNextFrame();
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

void RemoteDesktopWidget::keyPressEvent(QKeyEvent* event)
{
    if (!event->isAutoRepeat()) {
        emit sigKeyboardInputCaptured(KeyEventType::Press, static_cast<uint32_t>(event->nativeVirtualKey()));
    }
    event->accept();
    QWidget::keyPressEvent(event);
}

void RemoteDesktopWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (!event->isAutoRepeat()) {
        emit sigKeyboardInputCaptured(KeyEventType::Release, static_cast<uint32_t>(event->nativeVirtualKey()));
    }
    event->accept();
    QWidget::keyReleaseEvent(event);
}

void RemoteDesktopWidget::sendMouseEvent(MouseEventType type, const QPoint& localPos, int scrollDelta)
{
    if (m_currentFrame.isNull()) {
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

    emit sigMouseInputCaptured(type, mappedX, mappedY, scrollDelta);
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
