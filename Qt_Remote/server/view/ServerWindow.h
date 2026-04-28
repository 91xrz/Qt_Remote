#pragma once

#include <QWidget>

class QLineEdit;
class QPushButton;
class QTextEdit;
class QLabel;

class ServerWindow : public QWidget
{
    Q_OBJECT
public:
    explicit ServerWindow(QWidget* parent = nullptr);
    QString generatedPassword() const;

signals:
    void sigToggleListenRequested(quint16 port);
    void sigPasswordGenerated(const QString& password);

public slots:
    void updateStatus(const QString& statusText);
    void appendLog(const QString& message);
    void updateOnlineCount(int count);
    void updateListeningState(bool isListening, quint16 port);

private slots:
    void onToggleListenClicked();

private:
    QLineEdit* m_portEdit = nullptr;
    QPushButton* m_toggleButton = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_onlineLabel = nullptr;
    QLabel* m_passwordLabel = nullptr;
    QTextEdit* m_logView = nullptr;
    bool m_isListening = false;
    QString m_generatedPassword;
};
