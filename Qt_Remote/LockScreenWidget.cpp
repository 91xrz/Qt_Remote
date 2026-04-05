// LockScreenWidget.cpp
#include "LockScreenWidget.h"
#include "NetworkData.h"
#include <QVBoxLayout>
#include <QApplication>
#include <QScreen>
#include <atlimage.h>
LockScreenWidget::LockScreenWidget(QWidget* parent) : QWidget(parent) {
    // 1. 设置窗口标志：无边框 | 永远置顶 | 作为一个工具窗口（不在任务栏显示图标）
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::Tool);

    // 2. 设置纯黑背景
    setStyleSheet("background-color: black; color: red; font-size: 36px; font-weight: bold;");

    // 3. 居中显示提示文字
    m_lblMsg = new QLabel("设备已锁定", this);
    m_lblMsg->setAlignment(Qt::AlignCenter);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(m_lblMsg);
}

void LockScreenWidget::lock() {
    // 全屏显示 (完美替代 MFC 的 GetSystemMetrics 计算大小)
    this->showFullScreen();

    // 隐藏鼠标指针
    ShowCursor(false);

    // 锁定鼠标在屏幕正中心
    QScreen* screen = QApplication::primaryScreen();
    int cx = screen->geometry().width() / 2;
    int cy = screen->geometry().height() / 2;
    CRect rect(cx, cy, cx, cy);
    ClipCursor(&rect);

    // 隐藏任务栏
    HWND hTaskBar = FindWindow(L"Shell_TrayWnd", NULL);
    if (hTaskBar != NULL) {
        ShowWindow(hTaskBar, SW_HIDE);
    }
}

void LockScreenWidget::unlock() {
    this->hide(); // 隐藏全屏窗口

    // 恢复鼠标指针
    ShowCursor(true);

    // 解除鼠标范围锁定
    ClipCursor(NULL);

    // 恢复任务栏
    HWND hTaskBar = FindWindow(L"Shell_TrayWnd", NULL);
    if (hTaskBar != NULL) {
        ShowWindow(hTaskBar, SW_SHOW);
    }
}

void LockScreenWidget::keyPressEvent(QKeyEvent* event) {
    // 检查是否按下了 Insert 键
    if (event->key() == Qt::Key_Insert) {
        unlock();
        emit unlockedLocally(); // 通知外层业务逻辑：有人在本地解锁了
    }
}

void LockScreenWidget::closeEvent(QCloseEvent* event) {
    // 忽略关闭事件，彻底屏蔽 Alt+F4
    event->ignore();
}