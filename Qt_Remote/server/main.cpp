#include <QApplication>

#include "core/MainController.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    MainController controller;
    controller.show();

    return app.exec();
}
