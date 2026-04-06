#include "ServerWindow.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

#include "DeviceServer.h"

ServerWindow::ServerWindow(QWidget* parent)
    : QWidget(parent), m_server(new DeviceServer(this))
{
    setWindowTitle(QStringLiteral("Qt Remote Server (基础版)"));
    resize(700, 460);

    auto* mainLayout = new QVBoxLayout(this);
    auto* controlLayout = new QHBoxLayout();

    auto* portLabel = new QLabel(QStringLiteral("端口:"), this);
    m_portEdit = new QLineEdit(this);
    m_portEdit->setValidator(new QIntValidator(1, 65535, this));
    m_portEdit->setText(QStringLiteral("5555"));

    m_toggleButton = new QPushButton(QStringLiteral("启动监听"), this);

    m_statusLabel = new QLabel(QStringLiteral("状态: 未监听"), this);
    m_onlineLabel = new QLabel(QStringLiteral("在线客户端: 0"), this);

    controlLayout->addWidget(portLabel);
    controlLayout->addWidget(m_portEdit);
    controlLayout->addWidget(m_toggleButton);
    controlLayout->addStretch();

    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);

    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(m_onlineLabel);
    mainLayout->addWidget(m_logView, 1);

    connect(m_toggleButton, &QPushButton::clicked, this, &ServerWindow::onToggleListenClicked);

    connect(m_server, &DeviceServer::logMessage, this, &ServerWindow::appendLog);
    connect(m_server, &DeviceServer::statusChanged, this, [this](const QString& status) {
        m_statusLabel->setText(QStringLiteral("状态: %1").arg(status));
    });

    connect(m_server, &DeviceServer::clientConnected, this, [this](const QString& ip, quint16 port) {
        appendLog(QStringLiteral("连接事件: %1:%2").arg(ip).arg(port));
        refreshStatus();
    });

    connect(m_server, &DeviceServer::clientDisconnected, this, [this](const QString& ip, quint16 port) {
        appendLog(QStringLiteral("断开事件: %1:%2").arg(ip).arg(port));
        refreshStatus();
    });

    refreshStatus();
}

void ServerWindow::onToggleListenClicked()
{
    if (m_server->isListening()) {
        m_server->stopListen();
        refreshStatus();
        return;
    }

    const quint16 port = static_cast<quint16>(m_portEdit->text().toUShort());
    if (port == 0) {
        appendLog(QStringLiteral("请输入合法端口（1-65535）"));
        return;
    }

    if (m_server->startListen(port)) {
        refreshStatus();
    }
}

void ServerWindow::appendLog(const QString& message)
{
    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_logView->append(QStringLiteral("[%1] %2").arg(timestamp, message));
}

void ServerWindow::refreshStatus()
{
    if (m_server->isListening()) {
        m_toggleButton->setText(QStringLiteral("停止监听"));
        m_statusLabel->setText(QStringLiteral("状态: 监听中 (%1)").arg(m_server->listeningPort()));
    } else {
        m_toggleButton->setText(QStringLiteral("启动监听"));
        m_statusLabel->setText(QStringLiteral("状态: 未监听"));
    }

    m_onlineLabel->setText(QStringLiteral("在线客户端: %1").arg(m_server->sessionCount()));
}
