#include "Qt_Remote.h"
//#include "ClientSession.h"
#include "FileManagerWidget.h"
#include <QHostAddress>

Qt_Remote::Qt_Remote(QWidget *parent)
    : QMainWindow(parent)
    , m_fileManagerWidget(nullptr)
{
    ui.setupUi(this);

    ui.labelServiceStatusValue->setText(QStringLiteral("未连接"));
    ui.btnStopService->setEnabled(false);
    // 1. 初始化网络模块
    m_connection = new RemoteConnection(this);

    // 2. 绑定网络日志到 UI 
    connect(m_connection, &RemoteConnection::logMessage, this, [=](const QString& msg) {
        ui.plainTextLogs->appendPlainText(msg);
        });

    connect(m_connection, &RemoteConnection::connected, this, [=]() {
        ui.labelServiceStatusValue->setText(QStringLiteral("已连接"));
        ui.btnStartService->setEnabled(false);
        ui.btnStopService->setEnabled(true);
        ui.lineEditTargetIp->setEnabled(false);
        ui.lineEditPort->setEnabled(false);
        ui.plainTextLogs->appendPlainText(QStringLiteral("已连接到目标主机"));
        });

    connect(m_connection, &RemoteConnection::disconnected, this, [=]() {
        ui.labelServiceStatusValue->setText(QStringLiteral("未连接"));
        ui.btnStartService->setEnabled(true);
        ui.btnStopService->setEnabled(false);
        ui.lineEditTargetIp->setEnabled(true);
        ui.lineEditPort->setEnabled(true);
        ui.plainTextLogs->appendPlainText(QStringLiteral("已断开连接"));
        });

    connect(m_connection, &RemoteConnection::errorOccurred, this, [=](const QString& msg) {
        ui.plainTextLogs->appendPlainText(QStringLiteral("【网络错误】%1").arg(msg));
        ui.labelServiceStatusValue->setText(QStringLiteral("连接异常"));
        ui.btnStartService->setEnabled(true);
        ui.btnStopService->setEnabled(false);
        ui.lineEditTargetIp->setEnabled(true);
        ui.lineEditPort->setEnabled(true);
        });

    connect(ui.btnStartService, &QPushButton::clicked, this, [=]() {
        const QString ip = ui.lineEditTargetIp->text().trimmed();
        bool ok = false;
        const quint16 port = ui.lineEditPort->text().trimmed().toUShort(&ok);

        if (ip.isEmpty()) {
            ui.plainTextLogs->appendPlainText(QStringLiteral("【输入错误】目标IP不能为空"));
            return;
        }

        if (QHostAddress(ip).isNull()) {
            ui.plainTextLogs->appendPlainText(QStringLiteral("【输入错误】目标IP格式不正确：%1").arg(ip));
            return;
        }

        if (!ok || port == 0) {
            ui.plainTextLogs->appendPlainText(QStringLiteral("【输入错误】目标端口无效：%1").arg(ui.lineEditPort->text()));
            return;
        }

        ui.labelServiceStatusValue->setText(QStringLiteral("连接中..."));
        ui.plainTextLogs->appendPlainText(QStringLiteral("准备连接到 %1:%2").arg(ip).arg(port));
        m_connection->connectToServer(ip, port);
        });

    connect(ui.btnStopService, &QPushButton::clicked, this, [=]() {
        ui.plainTextLogs->appendPlainText(QStringLiteral("请求断开连接"));
        m_connection->disconnectFromServer();
        });
  
    //清空日志
    connect(ui.btnClearLogs, &QPushButton::clicked, this, [=]() {
        ui.plainTextLogs->clear();
        });

    //文件管理窗口
    connect(ui.btnFileManager, &QPushButton::clicked, this, [this]() {
        if (!m_fileManagerWidget) {
            m_fileManagerWidget = new FileManagerWidget(this);
            m_fileManagerWidget->setWindowFlags(Qt::Window);
            m_fileManagerWidget->setWindowTitle(QStringLiteral("远程文件管理器"));
            m_fileManagerWidget->resize(900, 560);
        }

        // 显示、置顶、激活
        m_fileManagerWidget->show();
        m_fileManagerWidget->raise();
        m_fileManagerWidget->activateWindow();
        });

    connect(ui.btnTest, &QPushButton::clicked, this, [=]() {
        if (!m_connection->isConnected()) {
            ui.plainTextLogs->appendPlainText(QStringLiteral("【功能测试】当前未连接，无法发送测试指令"));
            return;
        }
        ui.plainTextLogs->appendPlainText(QStringLiteral("【功能测试】准备发送测试指令（DriverInfo）"));
        m_connection->sendPacket(CmdType::DriverInfo);
        });
}

Qt_Remote::~Qt_Remote()
{}
