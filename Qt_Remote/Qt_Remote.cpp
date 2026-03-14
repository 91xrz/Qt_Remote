#include "Qt_Remote.h"
#include "ClientSession.h"

Qt_Remote::Qt_Remote(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    m_logic = new DeviceServer(this);

    connect(m_logic, &DeviceServer::logMessage, this, [=](const QString& msg) {
        ui.statusBar->showMessage(msg);
        ui.plainTextConnectionLogs->appendPlainText(msg);
    });

    connect(m_logic, &DeviceServer::clientConnected, this, [=](const QString& ip, int port) {
        ui.labelStatusConnectionValue->setText("已连接");
        ui.labelPeerOnlineValue->setText("在线");
        ui.plainTextConnectionLogs->appendPlainText(QString("发现新连接: %1:%2").arg(ip).arg(port));
    });

    connect(ui.btnConnect, &QPushButton::clicked, this, [=]() {
        const int port = ui.lineEditPort->text().toInt();
        m_logic->startListen(port > 0 ? port : 8888);
    });

    connect(ui.btnDisconnect, &QPushButton::clicked, this, [=]() {
        m_logic->stopListen();
        ui.labelStatusConnectionValue->setText("未连接");
        ui.labelPeerOnlineValue->setText("离线");
    });

    connect(ui.btnReconnect, &QPushButton::clicked, this, [=]() {
        m_logic->stopListen();
        const int port = ui.lineEditPort->text().toInt();
        m_logic->startListen(port > 0 ? port : 8888);
        ui.plainTextConnectionLogs->appendPlainText("执行重连操作");
    });

    connect(ui.btnStartControl, &QPushButton::clicked, this, [=]() {
        ui.plainTextPacketLogs->appendPlainText("开始远程控制");
    });

    connect(ui.btnStopControl, &QPushButton::clicked, this, [=]() {
        ui.plainTextPacketLogs->appendPlainText("停止远程控制");
    });

    connect(ui.btnLockMachine, &QPushButton::clicked, this, [=]() {
        ui.plainTextPacketLogs->appendPlainText("发送控制命令: 锁机");
    });

    connect(ui.btnUnlockMachine, &QPushButton::clicked, this, [=]() {
        ui.plainTextPacketLogs->appendPlainText("发送控制命令: 解锁");
    });

    connect(ui.btnShutdownMachine, &QPushButton::clicked, this, [=]() {
        ui.plainTextPacketLogs->appendPlainText("发送控制命令: 关机");
    });

    connect(ui.btnClearLogs, &QPushButton::clicked, this, [=]() {
        ui.plainTextConnectionLogs->clear();
        ui.plainTextPacketLogs->clear();
        ui.plainTextErrorLogs->clear();
    });

    connect(ui.btnExportLogs, &QPushButton::clicked, this, [=]() {
        ui.plainTextConnectionLogs->appendPlainText("导出日志: 功能预留");
    });
}

Qt_Remote::~Qt_Remote()
{}
