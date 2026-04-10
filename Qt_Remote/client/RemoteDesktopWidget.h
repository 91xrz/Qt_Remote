#pragma once

#include <QWidget>
#include <QPixmap>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPushButton>
#include "NetworkData.h"

class RemoteConnection;

class RemoteDesktopWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RemoteDesktopWidget(QWidget* parent = nullptr);

    void setConnection(RemoteConnection* conn);

public slots:
    void onScreenDataReceived(const QPixmap& pixmap);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void sendMouseEvent(MouseEventType type, const QPoint& localPos, int scrollDelta = 0);
    QRect remoteDisplayRect() const;
    bool isInRemoteDisplay(const QPoint& pos) const;
    void sendLockMachineCommand();
    void sendUnlockMachineCommand();

private:
    RemoteConnection* m_connection = nullptr;
    QPixmap m_currentFrame;
    QElapsedTimer m_mouseTimer;
    QPushButton* m_lockButton = nullptr;
    QPushButton* m_unlockButton = nullptr;
};
