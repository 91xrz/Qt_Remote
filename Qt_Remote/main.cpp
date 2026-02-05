#include "Qt_Remote.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Qt_Remote window;
    window.show(); 
    return app.exec();
}
