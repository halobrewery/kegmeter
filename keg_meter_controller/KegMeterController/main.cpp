#include "mainwindow.h"
#include "kegmeterdata.h"

#include <QApplication>
#include <QStyle>
#include <QDesktopWidget>

int main(int argc, char *argv[])
{
    qRegisterMetaTypeStreamOperators<KegMeterData>("KegMeterData");
    qRegisterMetaType<KegMeterData>();

    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("Halo Brewery");
    QCoreApplication::setOrganizationDomain("halobrewery.com");
    QCoreApplication::setApplicationName("keg Meter Controller");

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
