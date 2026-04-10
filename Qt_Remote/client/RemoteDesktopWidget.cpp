#include "RemoteDesktopWidget.h"

#include <QHBoxLayout>
#include <QTimer>
#include <QVBoxLayout>
#include <QEvent>

#include "RemoteConnection.h"

RemoteDesktopWidget::RemoteDesktopWidget(QWidget* parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    m_toolbarWidget = new QWidget(this);
    auto* toolbarLayout = new QHBoxLayout(m_toolbarWidget);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(8);

    m_btnLockMachine = new QPushButton(QStringLiteral("锁机"), m_toolbarWidget);
    m_btnUnlockMachine = new QPushButton(QStringLiteral("解锁"), m_toolbarWidget);
    m_btnLockMachine->setEnabled(false);
    m_btnUnlockMachine->setEnabled(false);

    toolbarLayout->addWidget(m_btnLockMachine);
    toolbarLayout->addWidget(m_btnUnlockMachine);
    toolbarLayout->addStretch();

    m_screenLabel = new QLabel(this);
    m_screenLabel->setAlignment(Qt::AlignCenter);
    m_screenLabel->setStyleSheet("background-color: black;");
    m_screenLabel->setScaledContents(true);
    m_screenLabel->setMouseTracking(true);
    m_screenLabel->installEventFilter(this);

    mainLayout->addWidget(m_toolbarWidget);
    mainLayout->addWidget(m_screenLabel, 1);

    connect(m_btnLockMachine, &QPushButton::clicked, this, &RemoteDesktopWidget::sigLockClicked);
    connect(m_btnUnlockMachine, &QPushButton::clicked, this, &RemoteDesktopWidget::sigUnlockClicked);

    m_mouseTimer.start();
}

void RemoteDesktopWidget::setConnection(RemoteConnection* conn)
{
    m_connection = conn;
}

void RemoteDesktopWidget::setLockButtonsEnabled(bool lockEnabled, bool unlockEnabled)
{
    if (m_btnLockMachine) {
        m_btnLockMachine->setEnabled(lockEnabled);
    }
    if (m_btnUnlockMachine) {
        m_btnUnlockMachine->setEnabled(unlockEnabled);
    }
}

void RemoteDesktopWidget::onScreenDataReceived(const QPixmap& pixmap)
{
    m_currentFrame = pixmap;
    m_screenLabel->setPixmap(m_currentFrame);

    if (m_connection && this->isVisible()) {
        QTimer::singleShot(30, this, [this]() {
            if (m_connection && this->isVisible()) {
                m_connection->sendPacket(CmdType::ScreenData);
            }
        });
    }
}

void RemoteDesktopWidget::closeEvent(QCloseEvent* event)
{
    m_currentFrame = QPixmap();
    m_screenLabel->clear();
    QWidget::closeEvent(event);
}

bool RemoteDesktopWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != m_screenLabel) {
        return QWidget::eventFilter(watched, event);
    }

    switch (event->type()) {
    case QEvent::MouseMove: {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (m_mouseTimer.elapsed() > 30) {
            sendMouseEvent(MouseEventType::Move, clampLocalPos(mouseEvent->pos()));
            m_mouseTimer.restart();
        }
        return false;
    }
    case QEvent::MouseButtonPress: {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        MouseEventType type = MouseEventType::None;
        switch (mouseEvent->button()) {
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
            sendMouseEvent(type, clampLocalPos(mouseEvent->pos()));
        }
        return false;
    }
    case QEvent::MouseButtonRelease: {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        MouseEventType type = MouseEventType::None;
        switch (mouseEvent->button()) {
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
            sendMouseEvent(type, clampLocalPos(mouseEvent->pos()));
        }
        return false;
    }
    case QEvent::MouseButtonDblClick: {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        MouseEventType type = MouseEventType::None;
        switch (mouseEvent->button()) {
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
            sendMouseEvent(type, clampLocalPos(mouseEvent->pos()));
        }
        return false;
    }
    case QEvent::Wheel: {
        auto* wheelEvent = static_cast<QWheelEvent*>(event);
        sendMouseEvent(MouseEventType::Scroll, clampLocalPos(wheelEvent->position().toPoint()), wheelEvent->angleDelta().y());
        return false;
    }
    default:
        return QWidget::eventFilter(watched, event);
    }
}

void RemoteDesktopWidget::sendMouseEvent(MouseEventType type, const QPoint& localPos, int scrollDelta)
{
    if (!m_connection || m_currentFrame.isNull() || !m_screenLabel) {
        return;
    }

    const int localW = m_screenLabel->width();
    const int localH = m_screenLabel->height();
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

QPoint RemoteDesktopWidget::clampLocalPos(const QPoint& localPos) const
{
    if (!m_screenLabel) {
        return localPos;
    }

    const int x = qBound(0, localPos.x(), qMax(0, m_screenLabel->width() - 1));
    const int y = qBound(0, localPos.y(), qMax(0, m_screenLabel->height() - 1));
    return QPoint(x, y);
}
