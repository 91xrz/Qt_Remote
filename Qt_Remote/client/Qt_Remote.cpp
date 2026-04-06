#include "Qt_Remote.h"
//#include "ClientSession.h"
#include "FileManagerWidget.h"
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
    , m_fileManagerWidget(nullptr)
{
    ui.setupUi(this);

    ui.labelLocalIpValue->setText(getLocalIpv4Address());
    ui.btnStopService->setEnabled(false);
    // 1. 初始化网络模块
    m_connection = new RemoteConnection(this);

    // 2. 绑定网络日志到 UI 
    connect(m_connection, &RemoteConnection::logMessage, this, [=](const QString& msg) {
        ui.plainTextLogs->appendPlainText(msg);
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
    /*
    m_logic = new DeviceServer(this);
 

    connect(m_logic, &DeviceServer::logMessage, this, [=](const QString& msg) {
        ui.statusBar->showMessage(msg);
        ui.plainTextLogs->appendPlainText(msg);
    });

    connect(m_logic, &DeviceServer::clientConnected, this, [=](const QString& ip, int port) {
        ui.plainTextLogs->appendPlainText(QString("客户端连接: %1:%2").arg(ip).arg(port));
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

   
    */
}

Qt_Remote::~Qt_Remote()
{}
