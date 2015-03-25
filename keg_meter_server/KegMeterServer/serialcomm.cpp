#include "serialcomm.h"
#include "mainwindow.h"
#include "kegmeter.h"
#include "appsettings.h"
#include "serialsearchandconnectdialog.h"

#include <QTextStream>
#include <QSettings>
#include <QSerialPortInfo>

SerialComm::SerialComm(MainWindow* mainWindow) :
    AbstractComm(mainWindow),
    serialPort(new QSerialPort()),
    bytesWritten(0) {

    this->serialPort->setBaudRate(QSerialPort::Baud9600);
    this->serialPort->setParity(QSerialPort::NoParity);
    this->serialPort->setStopBits(QSerialPort::OneStop);
    this->serialPort->setDataBits(QSerialPort::Data8);
    this->serialPort->setFlowControl(QSerialPort::NoFlowControl);

    this->serialConnDialog = new SerialSearchAndConnectDialog(this, this->mainWindow);

    this->connect(this->serialPort, SIGNAL(error(QSerialPort::SerialPortError)),
                  this, SLOT(onSerialPortError(QSerialPort::SerialPortError)));
    this->connect(this->serialPort, SIGNAL(aboutToClose()), this, SLOT(onSerialPortClose()));
    this->connect(this->serialPort, SIGNAL(readyRead()), this, SLOT(onSerialPortReadyRead()));
    this->connect(this->serialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(onSerialPortBytesWritten(qint64)));

    this->connect(&this->delayedSendTimer, SIGNAL(timeout()), this, SLOT(onDelayedSendTimer()));
    this->connect(&this->trySerialTimer, SIGNAL(timeout()), this, SLOT(onTrySerialTimer()));

    this->trySerialTimer.setSingleShot(true);
    this->trySerialTimer.start(TRY_SERIAL_TIMEOUT_MS);
    this->delayedSendTimer.setSingleShot(true);
}

SerialComm::~SerialComm() {
    this->trySerialTimer.stop();

    delete this->serialConnDialog;
    this->serialConnDialog = NULL;

    if (this->serialPort->isOpen()) {
        this->serialPort->close();
    }
    delete this->serialPort;
    this->serialPort = NULL;
}

void SerialComm::write(const QByteArray &data) {
    if (!this->serialPort->isOpen()) {
        return;
    }

    this->commWriteData = data;

    qint64 bytesWritten = this->serialPort->write(data);
    if (bytesWritten == -1) {
        this->mainWindow->log(tr("Failed to write the data to port %1, error: %2").arg(this->serialPort->portName()).arg(this->serialPort->errorString()));
    }
    else if (bytesWritten != data.size()) {
       this->mainWindow->log(tr("Failed to write all the data to port %1, error: %2").arg(this->serialPort->portName()).arg(this->serialPort->errorString()));
    }
    else {
        this->mainWindow->log(tr("Wrote serial data: ") + QString(data.toStdString().c_str()));
    }

    this->serialPort->flush();
}

void SerialComm::executeSettingsDialog() {
    this->trySerialTimer.stop();
    this->serialConnDialog->exec();
    if (!this->serialPort->isOpen()) {
        this->trySerialTimer.start(TRY_SERIAL_TIMEOUT_MS);
    }
}

void SerialComm::onTrySerialTimer() {
    if (this->serialPort->isOpen()) {
        return;
    }

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    if (!ports.empty()) {

        // Try to find a port that makes sense...
        foreach (const QSerialPortInfo& port, ports) {
            if (!port.isBusy() && port.isValid() &&
                 (port.manufacturer().contains("Arduino", Qt::CaseInsensitive) ||
                 port.description().contains("Arduino", Qt::CaseInsensitive))) {

                    this->openSerialPort(port);
                    break;
                }
            }

        if (!this->serialPort->isOpen()) {
            // Try to find a port that makes sense...
            foreach (const QSerialPortInfo& port, ports) {
                if (!port.isBusy() && port.isValid() &&
                        port.description().contains("AdafruitEZ", Qt::CaseInsensitive)) {

                    this->openSerialPort(port);
                    break;
                }
            }
        }
    }

    if (!this->serialPort->isOpen() && !this->trySerialTimer.isActive()) {
        this->trySerialTimer.start(TRY_SERIAL_TIMEOUT_MS);
    }
}

void SerialComm::onDelayedSendTimer() {
    auto kegMeters = this->mainWindow->getKegMeters();
    foreach (auto* kegMeter, kegMeters) {
        kegMeter->outputSync();
    }
}

void SerialComm::onSerialPortError(const QSerialPort::SerialPortError&) {
    QString errorMsg = this->serialPort->errorString();
    if (!errorMsg.isEmpty()) {
        this->mainWindow->log(errorMsg);
    }
    if (this->serialPort->isOpen()) {
        this->serialPort->close();
    }
}

void SerialComm::onSerialPortClose() {
    // Disable all of the meter GUIs
    auto kegMeters = this->mainWindow->getKegMeters();
    foreach (auto* kegMeter, kegMeters) {
        kegMeter->setEnabled(false);
    }

    if (!this->trySerialTimer.isActive()) {
        this->trySerialTimer.start(TRY_SERIAL_TIMEOUT_MS);
    }
}

void SerialComm::onSerialPortBytesWritten(qint64 bytes) {
    this->bytesWritten += bytes;
}

void SerialComm::onSerialPortReadyRead() {
    QByteArray readBytes = this->serialPort->readAll();
    this->mainWindow->commLog(readBytes);

    // We remember the full string coming in over serial, if an error is
    // detected we resend the last serial message!
    QString readStr = QString(readBytes.toStdString().c_str());
    this->tempRememberBuf += readStr;
    if (this->tempRememberBuf.contains("ERROR")) {
        this->tempRememberBuf.clear();
        this->write(this->commWriteData);
    }
    else if (this->tempRememberBuf.size() > 2048){
        this->tempRememberBuf.clear();
    }

    this->commReadData.append(readBytes);

    // Look through the read data for full packages
    while (true) {
        int startIdx = this->commReadData.indexOf('[');
        int endIdx   = this->commReadData.indexOf(']');

        if (startIdx == -1) {
            // No package fragments were found at all
            this->commReadData.clear();
            break;
        }
        if (endIdx == -1) {
            // Only part of a fragment was found, skim to the beginning and wait for the rest
            this->commReadData.remove(0, startIdx);
            break;
        }

        this->commReadData.remove(0, startIdx);
        if (endIdx < startIdx) {
            continue;
        }

        // We've removed everything up to the package in the serialReadData,
        // remap start and end indexes
        endIdx -= startIdx;
        startIdx = 0;

        // Make sure it's a valid package...
        int pkgLen = endIdx;
        if (pkgLen < 6) {
            this->commReadData.remove(0,1);
            continue;
        }

        QString pkgStr;
        for (int i = 1; i < pkgLen; i++) {
            pkgStr += this->commReadData.at(i);
        }

        QTextStream pkgTextStream(&pkgStr);

        int meterIdx;
        pkgTextStream >> meterIdx;
        if (pkgTextStream.status() != QTextStream::Ok || meterIdx >= this->mainWindow->getNumKegMeters() || meterIdx < 0) {
            this->commReadData.remove(0,1);
            continue;
        }

        char temp;
        pkgTextStream >> temp; // ' '
        if (pkgTextStream.status() != QTextStream::Ok || temp != ' ') {
            this->commReadData.remove(0,1);
            continue;
        }

        bool exitLoop = false;
        while (!exitLoop) {

            pkgTextStream >> temp;
            if (pkgTextStream.status() != QTextStream::Ok) { exitLoop = true; break; }

            switch (temp) {

            case 'M': {
                pkgTextStream >> temp; // ' '
                if (temp != ' ' || pkgTextStream.status() != QTextStream::Ok) { exitLoop = true; break; }

                float measurement = 0;
                pkgTextStream >> measurement;

                KegMeter* kegMeter = this->mainWindow->getKegMeters().at(meterIdx);
                assert(kegMeter != NULL);
                kegMeter->updateLoadMeasurement(measurement);

                exitLoop = true;
                break;
            }

            default:
                exitLoop = true;
                break;
            }
        }

        // Remove the package from the serialReadData -- we do this by just removing the start character
        this->commReadData.remove(0,1);
    }
}

void SerialComm::openSerialPort(const QSerialPortInfo& portInfo) {
    if (this->serialPort->isOpen()) {
        return;
    }

    this->serialPort->setPort(portInfo);
    if (this->serialPort->open(QIODevice::ReadWrite)) {
        this->mainWindow->log(tr("Connected to %1 @ %2 baud")
                  .arg(this->serialPort->portName())
                  .arg(this->serialPort->baudRate()));

        // Now that the serial port is open, offer the user the ability to restore any previous
        // known state for the keg meters
        this->delayedSendTimer.start(2000);
    }
    else {
        this->mainWindow->log(tr("Failed to connect to serial port %1").arg(portInfo.portName()));
    }
}
