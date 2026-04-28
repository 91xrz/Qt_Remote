#include "Qt_Remote.h"

#include <QHostAddress>

Qt_Remote::Qt_Remote(QWidget* parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    ui.labelServiceStatusValue->setText(QStringLiteral("未连接"));
    ui.btnStopService->setEnabled(false);

    connect(ui.btnStartService, &QPushButton::clicked, this, [this]() {
        const QString ip = ui.lineEditTargetIp->text().trimmed();
        const QString password = ui.lineEditPassword->text().trimmed();
        bool ok = false;
        const quint16 port = ui.lineEditPort->text().trimmed().toUShort(&ok);

        if (ip.isEmpty()) {
            appendLogMessage(QStringLiteral("【输入错误】目标IP不能为空"));
            return;
        }

        if (QHostAddress(ip).isNull()) {
            appendLogMessage(QStringLiteral("【输入错误】目标IP格式不正确：%1").arg(ip));
            return;
        }

        if (!ok || port == 0) {
            appendLogMessage(QStringLiteral("【输入错误】目标端口无效：%1").arg(ui.lineEditPort->text()));
            return;
        }

        if (password.isEmpty()) {
            appendLogMessage(QStringLiteral("【输入错误】验证码不能为空"));
            return;
        }

        emit sigConnectRequested(ip, port, password);
    });

    connect(ui.btnStopService, &QPushButton::clicked, this, [this]() {
        emit sigDisconnectRequested();
    });

    connect(ui.btnClearLogs, &QPushButton::clicked, this, [this]() {
        ui.plainTextLogs->clear();
    });

    connect(ui.btnFileManager, &QPushButton::clicked, this, [this]() {
        emit sigOpenFileManagerRequested();
    });

    connect(ui.btnRemoteDesktop, &QPushButton::clicked, this, [this]() {
        emit sigOpenRemoteDesktopRequested();
    });

    connect(ui.btnTest, &QPushButton::clicked, this, [this]() {
        emit sigTestRequested();
    });
}

Qt_Remote::~Qt_Remote() {}

void Qt_Remote::appendLogMessage(const QString& msg)
{
    ui.plainTextLogs->appendPlainText(msg);
}

void Qt_Remote::onConnected()
{
    ui.labelServiceStatusValue->setText(QStringLiteral("已连接"));
    ui.btnStartService->setEnabled(false);
    ui.btnStopService->setEnabled(true);
    ui.lineEditTargetIp->setEnabled(false);
    ui.lineEditPort->setEnabled(false);
    ui.lineEditPassword->setEnabled(false);
    appendLogMessage(QStringLiteral("已连接到目标主机"));
}

void Qt_Remote::onDisconnected()
{
    ui.labelServiceStatusValue->setText(QStringLiteral("未连接"));
    ui.btnStartService->setEnabled(true);
    ui.btnStopService->setEnabled(false);
    ui.lineEditTargetIp->setEnabled(true);
    ui.lineEditPort->setEnabled(true);
    ui.lineEditPassword->setEnabled(true);
    appendLogMessage(QStringLiteral("已断开连接"));
}

void Qt_Remote::onConnectionError(const QString& msg)
{
    appendLogMessage(QStringLiteral("【网络错误】%1").arg(msg));
    ui.labelServiceStatusValue->setText(QStringLiteral("连接异常"));
    ui.btnStartService->setEnabled(true);
    ui.btnStopService->setEnabled(false);
    ui.lineEditTargetIp->setEnabled(true);
    ui.lineEditPort->setEnabled(true);
    ui.lineEditPassword->setEnabled(true);
}

void Qt_Remote::setConnectingState(const QString& ip, quint16 port)
{
    ui.labelServiceStatusValue->setText(QStringLiteral("连接中..."));
    appendLogMessage(QStringLiteral("准备连接到 %1:%2").arg(ip).arg(port));
}
