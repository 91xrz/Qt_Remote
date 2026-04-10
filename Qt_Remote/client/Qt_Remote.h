#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Qt_Remote.h"
#include "RemoteConnection.h"
#include "ClientCommandHandler.h"

class FileManagerWidget;
class RemoteDesktopWidget;

#pragma execution_character_set("utf-8")
// 视图层
class Qt_Remote : public QMainWindow
{
    Q_OBJECT

public:
    Qt_Remote(QWidget *parent = nullptr);
    ~Qt_Remote();

private:
    Ui::Qt_RemoteClass ui;
    RemoteConnection* m_connection = nullptr;
    FileManagerWidget* m_fileManagerWidget = nullptr;
    RemoteDesktopWidget* m_desktopWidget = nullptr;
	ClientCommandHandler* m_commandHandler = nullptr;
    bool m_isRemoteLocked = false;

    void updateLockButtons();

private slots:

};
