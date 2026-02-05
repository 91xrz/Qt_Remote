#include "Qt_Remote.h"
#include "ClientSession.h"
Qt_Remote::Qt_Remote(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    // 1. 创建逻辑对象
    m_logic = new DeviceServer(this);

    // 2. 连接信号：逻辑层发生什么事，界面层负责显示
    connect(m_logic, &DeviceServer::logMessage, this, [=](QString msg) {
        ui.statusBar->showMessage(msg); // 更新状态栏
        });

    connect(m_logic, &DeviceServer::clientConnected, this, [=](QString ip, int port) {
        QMessageBox::information(this, "提示", "发现新连接: " + ip);
        });

    // 3. 界面操作逻辑：点击按钮启动服务
    connect(ui.pushButton, &QPushButton::clicked, this, [=]() {
        m_logic->startListen(8888);
        });
}

Qt_Remote::~Qt_Remote()
{}

