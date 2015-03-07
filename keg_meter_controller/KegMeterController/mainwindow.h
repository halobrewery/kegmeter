#ifndef KEGMETERCONTROLLER_MAINWINDOW_H
#define KEGMETERCONTROLLER_MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QtSerialPort/QSerialPort>

class KegMeter;
class PreferencesDialog;

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

private slots:
    void onPreferencesActionTriggered();
    void onSerialInfoActionTriggered();

    void onSelectedSerialPortError(const QSerialPort::SerialPortError& error);
    void onSelectedSerialPortClose();
    void onSelectedSerialPortReadyRead();
    void onSelectedSerialPortBytesWritten(qint64 bytes);

    void onEmptyCalibration(const KegMeter& kegMeter);

    void onTrySerialTimer();

private:
    Ui::MainWindow* ui;
    QDialog* serialInfoDialog;
    PreferencesDialog* prefDialog;

    QSerialPort* selectedSerialPort;
    QByteArray   serialReadData;
    qint64 bytesWritten;

    static const int TRY_SERIAL_TIMEOUT_MS = 1000;
    QTimer trySerialTimer;

    static const int NUM_KEG_METERS = 8;
    QList<KegMeter*> kegMeters;

    void serialWrite(const QByteArray &writeData);
    void openSerialPort(const QSerialPortInfo& portInfo);
};

#endif // KEGMETERCONTROLLER_MAINWINDOW_H
