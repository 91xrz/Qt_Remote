#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QTextEdit;
class QLabel;
class DeviceServer;

class ServerWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ServerWindow(QWidget* parent = nullptr);

private slots:
    void onToggleListenClicked();
    void appendLog(const QString& message);
    void refreshStatus();

private:
    QLineEdit* m_portEdit = nullptr;
    QPushButton* m_toggleButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_onlineLabel = nullptr;
    QTextEdit* m_logView = nullptr;
    DeviceServer* m_server = nullptr;
};
