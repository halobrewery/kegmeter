#ifndef KEGMETERCONTROLLER_MAINWINDOW_H
#define KEGMETERCONTROLLER_MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QtSerialPort/QSerialPort>

class KegMeter;
class SerialSearchAndConnectDialog;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void log(const QString& logStr, bool newLine = true);
    void serialLog(const QString& logStr);

    void openSerialPort(const QSerialPortInfo& portInfo);

public slots:
    void onTrySerialTimer();

private slots:
    void onDelayedSendTimer();
    void onSerialSearchAndConnectDialogActionTriggered();
    void onSerialInfoActionTriggered();

    void onSelectedSerialPortError(const QSerialPort::SerialPortError& error);
    void onSelectedSerialPortClose();
    void onSelectedSerialPortReadyRead();
    void onSelectedSerialPortBytesWritten(qint64 bytes);

    void onEmptyCalibration(const KegMeter& kegMeter);

private:
    Ui::MainWindow* ui;
    QDialog* serialInfoDialog;
    SerialSearchAndConnectDialog* serialConnDialog;

    QSerialPort* selectedSerialPort;
    QByteArray   serialReadData;
    qint64 bytesWritten;

    static const int TRY_SERIAL_TIMEOUT_MS = 1000;
    QTimer trySerialTimer;
    QTimer delayedSendTimer;

    static const int NUM_KEG_METERS = 8;
    QList<KegMeter*> kegMeters;

    static const char* KEG_DATA_KEY;

    void serialWrite(const QByteArray &writeData, bool flush = true);
    void writeKegMeterDataToSettings() const;
};

#endif // KEGMETERCONTROLLER_MAINWINDOW_H
