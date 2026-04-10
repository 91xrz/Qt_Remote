#pragma once

#include <QWidget>
#include <QPixmap>
#include <QElapsedTimer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLabel>
#include <QPushButton>
#include "NetworkData.h"

class RemoteConnection;

class RemoteDesktopWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RemoteDesktopWidget(QWidget* parent = nullptr);

    void setConnection(RemoteConnection* conn);
    void setLockButtonsEnabled(bool lockEnabled, bool unlockEnabled);

public slots:
    void onScreenDataReceived(const QPixmap& pixmap);

signals:
    void sigLockClicked();
    void sigUnlockClicked();

protected:
    void closeEvent(QCloseEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void sendMouseEvent(MouseEventType type, const QPoint& localPos, int scrollDelta = 0);
    QPoint clampLocalPos(const QPoint& localPos) const;

private:
    RemoteConnection* m_connection = nullptr;
    QPixmap m_currentFrame;
    QElapsedTimer m_mouseTimer;
    QWidget* m_toolbarWidget = nullptr;
    QLabel* m_screenLabel = nullptr;
    QPushButton* m_btnLockMachine = nullptr;
    QPushButton* m_btnUnlockMachine = nullptr;
};
