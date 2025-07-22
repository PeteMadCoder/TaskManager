#include <QApplication>
#include "mainwindow.h"
#include "database.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Set application icon
    app.setWindowIcon(QIcon(":/resources/app_icon.png"));
    
    MainWindow mainWindow;
    mainWindow.show();
    
    int ret = app.exec();
    Database::shutdown();
    return ret;
}