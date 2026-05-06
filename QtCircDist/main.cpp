#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // ”станавливаем глобальный стиль дл€ единообрази€
    app.setStyle("Fusion");

    MainWindow w;
    w.show();
    return app.exec();
}
