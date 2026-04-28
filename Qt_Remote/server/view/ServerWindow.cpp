#include "view/ServerWindow.h"

#include <QDateTime>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QTextEdit>
#include <QVBoxLayout>

ServerWindow::ServerWindow(QWidget* parent)
    : QWidget(parent)
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
    m_generatedPassword = QString("%1")
        .arg(QRandomGenerator::global()->bounded(1000000), 6, 10, QLatin1Char('0'));
    m_passwordLabel = new QLabel(QStringLiteral("本机验证码: %1").arg(m_generatedPassword), this);

    controlLayout->addWidget(portLabel);
    controlLayout->addWidget(m_portEdit);
    controlLayout->addWidget(m_toggleButton);
    controlLayout->addStretch();

    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);

    mainLayout->addLayout(controlLayout);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(m_onlineLabel);
    mainLayout->addWidget(m_passwordLabel);
    mainLayout->addWidget(m_logView, 1);

    connect(m_toggleButton, &QPushButton::clicked, this, &ServerWindow::onToggleListenClicked);
    emit sigPasswordGenerated(m_generatedPassword);
}

QString ServerWindow::generatedPassword() const
{
    return m_generatedPassword;
}

void ServerWindow::onToggleListenClicked()
{
    const quint16 port = static_cast<quint16>(m_portEdit->text().toUShort());
    if (port == 0) {
        appendLog(QStringLiteral("请输入合法端口（1-65535）"));
        return;
    }
    emit sigToggleListenRequested(port);
}

void ServerWindow::updateStatus(const QString& statusText)
{
    m_statusLabel->setText(QStringLiteral("状态: %1").arg(statusText));
}

void ServerWindow::appendLog(const QString& message)
{
    const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    m_logView->append(QStringLiteral("[%1] %2").arg(timestamp, message));
}

void ServerWindow::updateOnlineCount(int count)
{
    m_onlineLabel->setText(QStringLiteral("在线客户端: %1").arg(count));
}

void ServerWindow::updateListeningState(bool isListening, quint16 port)
{
    m_isListening = isListening;
    if (m_isListening) {
        m_toggleButton->setText(QStringLiteral("停止监听"));
        m_statusLabel->setText(QStringLiteral("状态: 监听中 (%1)").arg(port));
    } else {
        m_toggleButton->setText(QStringLiteral("启动监听"));
        m_statusLabel->setText(QStringLiteral("状态: 未监听"));
    }
}
