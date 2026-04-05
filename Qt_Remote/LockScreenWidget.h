// LockScreenWidget.h
#pragma once
#include <QWidget>
#include <QLabel>
#include <QKeyEvent>
#include <windows.h>

class LockScreenWidget : public QWidget {
    Q_OBJECT
public:
    explicit LockScreenWidget(QWidget* parent = nullptr);
    void lock();
    void unlock();

signals:
    // 如果用户在被控端本地按下了 Insert 键解锁，触发此信号
    void unlockedLocally();

protected:
    // 拦截键盘事件，实现 Insert 键解锁
    void keyPressEvent(QKeyEvent* event) override;
    // 拦截关闭事件，防止用户按 Alt+F4 关掉锁机窗口
    void closeEvent(QCloseEvent* event) override;

private:
    QLabel* m_lblMsg;
};