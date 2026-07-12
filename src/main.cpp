#include "app/main_window.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    tracegraph::app::MainWindow window;
    window.resize(1200, 800);
    window.show();

    return application.exec();
}
