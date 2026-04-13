#pragma once

#include <QElapsedTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPixmap>
#include <QPushButton>
#include <QWheelEvent>
#include <QWidget>

#include "NetworkData.h"

class RemoteDesktopWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RemoteDesktopWidget(QWidget* parent = nullptr);

public slots:
    void updateFrame(const QPixmap& pixmap);

signals:
    void sigRequestNextFrame();
    void sigMouseInputCaptured(MouseEventType eventType, int x, int y, int scrollDelta);
    void sigKeyboardInputCaptured(KeyEventType eventType, uint32_t vkCode);
    void sigLockMachineRequested();
    void sigUnlockMachineRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    void sendMouseEvent(MouseEventType type, const QPoint& localPos, int scrollDelta = 0);
    QRect remoteDisplayRect() const;
    bool isInRemoteDisplay(const QPoint& pos) const;

private:
    QPixmap m_currentFrame;
    QElapsedTimer m_mouseTimer;
    QPushButton* m_lockButton = nullptr;
    QPushButton* m_unlockButton = nullptr;
};
