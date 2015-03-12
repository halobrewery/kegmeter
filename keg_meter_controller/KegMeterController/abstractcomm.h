#ifndef KEGMETERCONTROLLER_ABSTRACTCOMM_H
#define KEGMETERCONTROLLER_ABSTRACTCOMM_H

#include <cassert>
#include <QObject>
#include <QString>
#include <QByteArray>

#include "kegmeterdata.h"

class MainWindow;

class AbstractComm : public QObject {
    Q_OBJECT
public:
    AbstractComm(MainWindow* mainWindow) : mainWindow(mainWindow) { assert(mainWindow != NULL); }
    virtual ~AbstractComm() {}

    void writeString(const QString &data) { this->write(QByteArray(data.toStdString().c_str())); }

    virtual void write(const QByteArray &data) = 0;
    virtual void executeSettingsDialog() = 0;

signals:
    void kegMeterDataAvailable(const KegMeterData& data);
    void commClosed();

protected:
    MainWindow* mainWindow;
};

#endif // KEGMETERCONTROLLER_ABSTRACTCOMM_H

