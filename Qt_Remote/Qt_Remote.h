#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_Qt_Remote.h"

class Qt_Remote : public QMainWindow
{
    Q_OBJECT

public:
    Qt_Remote(QWidget *parent = nullptr);
    ~Qt_Remote();

private:
    Ui::Qt_RemoteClass ui;
};

