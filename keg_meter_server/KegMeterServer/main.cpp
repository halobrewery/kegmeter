#include "mainwindow.h"

#include <QApplication>
#include <QStyle>
#include <QDesktopWidget>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Halo Brewery");
    QCoreApplication::setOrganizationDomain("halobrewery.com");
    QCoreApplication::setApplicationName("Keg Meter Server");

    MainWindow w;
    w.show();
    w.setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            w.size(),
            qApp->desktop()->availableGeometry()
        ));

    return a.exec();
}
