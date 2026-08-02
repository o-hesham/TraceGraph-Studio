#include "app/main_window.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    QCoreApplication::setOrganizationName(QStringLiteral("TraceGraphStudio"));
    QCoreApplication::setApplicationName(QStringLiteral("TraceGraph Studio"));

    tracegraph::app::MainWindow window;
    window.show();

    return application.exec();
}
