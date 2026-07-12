#include "app/main_window.h"

#include <QTest>

class MainWindowTest final : public QObject
{
    Q_OBJECT

private slots:
    void hasProductTitle();
};

void MainWindowTest::hasProductTitle()
{
    tracegraph::app::MainWindow window;

    QCOMPARE(window.windowTitle(), QStringLiteral("TraceGraph Studio"));
}

QTEST_MAIN(MainWindowTest)

#include "main_window_test.moc"
