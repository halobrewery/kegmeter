#ifndef KEGMETERCONTROLLER_SERIALCOMM_H
#define KEGMETERCONTROLLER_SERIALCOMM_H

#include "abstractcomm.h"
#include <QSerialPort>
#include <QTimer>

class SerialSearchAndConnectDialog;

class SerialComm : public AbstractComm {
    Q_OBJECT
public:
    SerialComm(MainWindow* mainWindow);
    ~SerialComm();

    QSerialPort* getSerialPort() const { return this->serialPort; }
    void openSerialPort(const QSerialPortInfo& portInfo);

    void write(const QByteArray &data) override;
    void executeSettingsDialog() override;

public slots:
    void onTrySerialTimer();
private slots:
    void onDelayedSendTimer();
    void onSerialPortError(const QSerialPort::SerialPortError& error);
    void onSerialPortClose();
    void onSerialPortReadyRead();
    void onSerialPortBytesWritten(qint64 bytes);

private:
    QSerialPort* serialPort;

    // Cached read/write data
    QByteArray commReadData;
    QByteArray commWriteData;
    qint64 bytesWritten;
    QString tempRememberBuf;

    static const int TRY_SERIAL_TIMEOUT_MS = 1000;
    QTimer trySerialTimer;
    QTimer delayedSendTimer;

    SerialSearchAndConnectDialog* serialConnDialog;
};

#endif // KEGMETERCONTROLLER_SERIALCOMM_H
