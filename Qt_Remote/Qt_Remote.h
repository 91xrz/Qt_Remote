#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Qt_Remote.h"
#include "DeviceServer.h"
#include <QMessageBox>
#include <QTcpServer>
#include <QTcpSocket>   



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
    DeviceServer* m_logic; // 持有逻辑对象
    //依赖注入 qt与视频不同 暂时不采用单例 后续用到使用依赖注入
private slots:
	
};

