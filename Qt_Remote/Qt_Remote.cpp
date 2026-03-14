#include "Qt_Remote.h"
#include "ClientSession.h"
#include <QNetworkInterface>

namespace {
QString getLocalIpv4Address()
{
    const auto interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface& iface : interfaces) {
        if (!(iface.flags() & QNetworkInterface::IsUp) ||
            !(iface.flags() & QNetworkInterface::IsRunning) ||
            (iface.flags() & QNetworkInterface::IsLoopBack)) {
            continue;
        }

        const auto entries = iface.addressEntries();
        for (const QNetworkAddressEntry& entry : entries) {
            const QHostAddress address = entry.ip();
            if (address.protocol() == QAbstractSocket::IPv4Protocol) {
                return address.toString();
            }
        }
    }

    return QStringLiteral("127.0.0.1");
}
}

Qt_Remote::Qt_Remote(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    m_logic = new DeviceServer(this);
    ui.labelLocalIpValue->setText(getLocalIpv4Address());
    ui.btnStopService->setEnabled(false);

    connect(m_logic, &DeviceServer::logMessage, this, [=](const QString& msg) {
        ui.statusBar->showMessage(msg);
        ui.plainTextLogs->appendPlainText(msg);
    });

    connect(m_logic, &DeviceServer::clientConnected, this, [=](const QString& ip, int port) {
        ui.plainTextLogs->appendPlainText(QString("客户端连接: %1:%2").arg(ip).arg(port));
    });

    connect(ui.btnStartService, &QPushButton::clicked, this, [=]() {
        const int port = ui.lineEditPort->text().toInt();
        const int actualPort = port > 0 ? port : 8888;
        if (m_logic->startListen(actualPort)) {
            ui.labelServiceStatusValue->setText("运行中");
            ui.btnStartService->setEnabled(false);
            ui.btnStopService->setEnabled(true);
            ui.lineEditPort->setEnabled(false);
        }
    });

    connect(ui.btnStopService, &QPushButton::clicked, this, [=]() {
        m_logic->stopListen();
        ui.labelServiceStatusValue->setText("未启动");
        ui.btnStartService->setEnabled(true);
        ui.btnStopService->setEnabled(false);
        ui.lineEditPort->setEnabled(true);
    });

    connect(ui.btnTest, &QPushButton::clicked, this, [=]() {
        m_logic->testFunction();
        });
  



    connect(ui.btnClearLogs, &QPushButton::clicked, this, [=]() {
        ui.plainTextLogs->clear();
    });
}

Qt_Remote::~Qt_Remote()
{}
