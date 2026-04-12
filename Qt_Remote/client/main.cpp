#include "core/MainController.h"
#include <QtWidgets/QApplication>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    MainController controller;
    controller.show();
    return app.exec();
}
