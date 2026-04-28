#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Qt_Remote.h"

#pragma execution_character_set("utf-8")

class Qt_Remote : public QMainWindow
{
    Q_OBJECT

public:
    Qt_Remote(QWidget* parent = nullptr);
    ~Qt_Remote();

public slots:
    void appendLogMessage(const QString& msg);
    void onConnected();
    void onDisconnected();
    void onConnectionError(const QString& msg);
    void setConnectingState(const QString& ip, quint16 port);

signals:
    void sigConnectRequested(const QString& ip, quint16 port, const QString& password);
    void sigDisconnectRequested();
    void sigOpenFileManagerRequested();
    void sigOpenRemoteDesktopRequested();
    void sigTestRequested();

private:
    Ui::Qt_RemoteClass ui;
};
