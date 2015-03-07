#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "kegmeter.h"
#include "kegmeterdata.h"
#include "preferencesdialog.h"

#include <cassert>
#include <cmath>

#include <QVBoxLayout>
#include <QSpacerItem>
#include <QDialog>
#include <QSerialPortInfo>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow()),
    selectedSerialPort(new QSerialPort()),
    bytesWritten(0) {

    this->ui->setupUi(this);

    QHBoxLayout* mainLayout = new QHBoxLayout();

    const int KEG_METERS_PER_COL = 3;
    int numLoops = ceil(static_cast<float>(NUM_KEG_METERS) / static_cast<float>(KEG_METERS_PER_COL));
    for (int i = 0; i < numLoops; i++) {
        QVBoxLayout* vLayout = new QVBoxLayout();
        for (int j = i*KEG_METERS_PER_COL; j < i*KEG_METERS_PER_COL+KEG_METERS_PER_COL; j++) {

            if (j >= NUM_KEG_METERS) {
                vLayout->addItem(new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding));
                break;
            }
            else {
                KegMeter* meter = new KegMeter(j+1, this);
                this->kegMeters.push_back(meter);
                this->connect(meter, SIGNAL(doEmptyCalibration(KegMeter)), this, SLOT(onEmptyCalibration(KegMeter)));
                vLayout->addWidget(meter);
            }
        }
        mainLayout->addLayout(vLayout);
    }

    this->ui->centralWidget->setLayout(mainLayout);
    this->setWindowTitle("Halo Keg Meter Control Panel");

    this->selectedSerialPort->setBaudRate(115200);

    this->connect(this->ui->serialInfoAction, SIGNAL(triggered()), this, SLOT(onSerialInfoActionTriggered()));
    this->connect(this->ui->preferencesAction, SIGNAL(triggered()), this, SLOT(onPreferencesActionTriggered()));

    this->connect(this->selectedSerialPort, SIGNAL(error(QSerialPort::SerialPortError)),
                  this, SLOT(onSelectedSerialPortError(QSerialPort::SerialPortError)));
    this->connect(this->selectedSerialPort, SIGNAL(aboutToClose()), this, SLOT(onSelectedSerialPortClose()));
    this->connect(this->selectedSerialPort, SIGNAL(readyRead()), this, SLOT(onSelectedSerialPortReadyRead()));
    this->connect(this->selectedSerialPort, SIGNAL(bytesWritten(qint64)), this, SLOT(onSelectedSerialPortBytesWritten(qint64)));

    this->connect(&this->trySerialTimer, SIGNAL(timeout()), this, SLOT(onTrySerialTimer()));

    this->prefDialog = new PreferencesDialog(this->selectedSerialPort, this);

    this->serialInfoDialog = new QDialog(this);
    this->serialInfoDialog->setFixedSize(375, 400);
    this->serialInfoDialog->setWindowTitle("Serial Port Info");

    this->trySerialTimer.setSingleShot(true);
    this->trySerialTimer.start(TRY_SERIAL_TIMEOUT_MS);
}

MainWindow::~MainWindow() {
    this->trySerialTimer.stop();

    delete this->prefDialog;
    this->prefDialog = NULL;
    delete this->serialInfoDialog;
    this->serialInfoDialog = NULL;

    delete this->ui;
    this->ui = NULL;

    if (this->selectedSerialPort->isOpen()) {
        this->selectedSerialPort->close();
    }
    delete this->selectedSerialPort;
    this->selectedSerialPort = NULL;
}

void MainWindow::log(const QString& logStr, bool newLine) {
    this->ui->appLogTextEdit->insertPlainText(logStr + (newLine ? tr("\n") : tr("")));
    this->ui->appLogTextEdit->verticalScrollBar()->setValue(this->ui->appLogTextEdit->verticalScrollBar()->maximum());
}

void MainWindow::serialLog(const QString& logStr) {
    this->ui->serialLogTextEdit->insertPlainText(logStr);
    this->ui->serialLogTextEdit->verticalScrollBar()->setValue(this->ui->serialLogTextEdit->verticalScrollBar()->maximum());
}

void MainWindow::onPreferencesActionTriggered() {
    this->prefDialog->exec();
}

void MainWindow::onSerialInfoActionTriggered() {

    QVBoxLayout* textLayout = new QVBoxLayout();
    foreach (const QSerialPortInfo &info, QSerialPortInfo::availablePorts()) {
        QString s = QObject::tr("Port: ") + info.portName() + "\n"
                + QObject::tr("Location: ") + info.systemLocation() + "\n"
                + QObject::tr("Description: ") + info.description() + "\n"
                + QObject::tr("Manufacturer: ") + info.manufacturer() + "\n"
                + QObject::tr("Serial number: ") + info.serialNumber() + "\n"
                + QObject::tr("Vendor Identifier: ") + (info.hasVendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : QString()) + "\n"
                + QObject::tr("Product Identifier: ") + (info.hasProductIdentifier() ? QString::number(info.productIdentifier(), 16) : QString()) + "\n"
                + QObject::tr("Busy: ") + (info.isBusy() ? QObject::tr("Yes") : QObject::tr("No")) + "\n";

        QLabel *label = new QLabel(s);
        textLayout->addWidget(label);
    }

    QVBoxLayout* topLayout = new QVBoxLayout(this->serialInfoDialog);

    QWidget* temp = new QWidget();
    temp->setLayout(textLayout);

    QScrollArea* scrollArea = new QScrollArea(this->serialInfoDialog);
    scrollArea->setWidget(temp);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    topLayout->addWidget(scrollArea);

    this->serialInfoDialog->setLayout(topLayout);
    this->serialInfoDialog->show();
}

void MainWindow::onSelectedSerialPortError(const QSerialPort::SerialPortError& error) {
    switch (error) {
    case QSerialPort::DeviceNotFoundError:
        this->log(tr("An error occurred while attempting to open an non-existing device."));
        break;
    case QSerialPort::PermissionError:
        this->log(tr("An error occurred while attempting to open an already opened device by another process or a user not having enough permission and credentials to open."));
        break;
    case QSerialPort::OpenError:
        this->log(tr("An error occurred while attempting to open an already opened device in this object."));
        break;
    case QSerialPort::NotOpenError:
        this->log(tr("This error occurs when an operation is executed that can only be successfully performed if the device is open. This value was introduced in QtSerialPort 5.2."));
        break;
    case QSerialPort::ParityError:
        this->log(tr("Parity error detected by the hardware while reading data."));
        break;
    case QSerialPort::FramingError:
        this->log(tr("Framing error detected by the hardware while reading data."));
        break;
    case QSerialPort::BreakConditionError:
        this->log(tr("Break condition detected by the hardware on the input line."));
        break;
    case QSerialPort::WriteError:
        this->log(tr("An I/O error occurred while writing the data."));
        break;
    case QSerialPort::ReadError:
        this->log(tr("An I/O error occurred while reading the data."));
        break;
    case QSerialPort::ResourceError:
        this->log(tr("An I/O error occurred when a resource becomes unavailable, e.g. when the device is unexpectedly removed from the system."));
        break;
    case QSerialPort::UnsupportedOperationError:
        this->log(tr("The requested device operation is not supported or prohibited by the running operating system."));
        break;
    case QSerialPort::TimeoutError:
        this->log(tr("A timeout error occurred. This value was introduced in QtSerialPort 5.2."));
        break;
    case QSerialPort::UnknownError:
        this->log(tr("An unidentified error occurred."));
        break;

    default:
        return;
    }
    if (this->selectedSerialPort->isOpen()) {
        this->selectedSerialPort->close();
    }
}

void MainWindow::onSelectedSerialPortClose() {
    // Disable all of the meter GUIs
    foreach (KegMeter* meter, this->kegMeters) {
        meter->setEnabled(false);
    }

    if (!this->trySerialTimer.isActive()) {
        this->trySerialTimer.start(TRY_SERIAL_TIMEOUT_MS);
    }

}

void MainWindow::onSelectedSerialPortReadyRead() {
    QByteArray readBytes = this->selectedSerialPort->readAll();
    this->serialLog(readBytes);

    this->serialReadData.append(readBytes);


    // Look through the read data for full packages
    while (true) {
        int startIdx = this->serialReadData.indexOf('[');
        int endIdx   = this->serialReadData.indexOf(']');

        if (startIdx == -1) {
            // No package fragments were found at all
            this->serialReadData.clear();
            break;
        }
        if (endIdx == -1) {
            // Only part of a fragment was found, skim to the beginning and wait for the rest
            this->serialReadData.remove(0, startIdx);
            break;
        }

        this->serialReadData.remove(0, startIdx);
        if (endIdx < startIdx) {
            continue;
        }

        // We've removed everything up to the package in the serialReadData,
        // remap start and end indexes
        endIdx -= startIdx;
        startIdx = 0;

        // Make sure it's a valid package...
        int pkgLen = endIdx;
        if (pkgLen < 8) {
            this->serialReadData.remove(0,1);
            continue;
        }

        QString pkgStr;
        for (int i = 1; i < pkgLen; i++) {
            pkgStr += this->serialReadData.at(i);
        }

        QTextStream pkgTextStream(&pkgStr);

        // Start of package signature "<<<"


#define CHECK_SIGNATURE_CHAR() pkgTextStream >> temp; \
    if (pkgTextStream.status() != QTextStream::Ok || temp != '<') { \
        this->serialReadData.remove(0,1); continue; \
    }

        char temp;
        CHECK_SIGNATURE_CHAR();
        CHECK_SIGNATURE_CHAR();
        CHECK_SIGNATURE_CHAR();

        int id;
        pkgTextStream >> id;
        if (pkgTextStream.status() != QTextStream::Ok || id >= this->kegMeters.size() || id < 0) {
            this->serialReadData.remove(0,1);
            continue;
        }

        KegMeterData data(id);

        pkgTextStream >> temp; // '{'
        if (pkgTextStream.status() != QTextStream::Ok || temp != '{') {
            this->serialReadData.remove(0,1);
            continue;
        }

        bool exitLoop = false;
        bool success = false;
        while (!exitLoop && !success) {

            pkgTextStream >> temp;
            if (pkgTextStream.status() != QTextStream::Ok) { exitLoop = true; break; }

            switch (temp) {
            case 'P': {
                pkgTextStream >> temp; // ':'
                if (temp != ':' || pkgTextStream.status() != QTextStream::Ok) { exitLoop = true; break; }
                float percent = 0;
                pkgTextStream >> percent;
                data.setPercent(percent);
                break;
            }

            case 'F': {
                pkgTextStream >> temp; // ':'
                if (temp != ':' || pkgTextStream.status() != QTextStream::Ok) { exitLoop = true; break; }

                float fullMass = 0;
                pkgTextStream >> fullMass;
                data.setFullMass(fullMass);

                break;
            }

            case 'E': {
                pkgTextStream >> temp; // ':'
                if (temp != ':' || pkgTextStream.status() != QTextStream::Ok) { exitLoop = true; break; }

                float emptyMass = 0;
                pkgTextStream >> emptyMass;
                data.setEmptyMass(emptyMass);

                break;
            }

            case 'L': {
                pkgTextStream >> temp; // ':'
                if (temp != ':' || pkgTextStream.status() != QTextStream::Ok) { exitLoop = true; break; }

                float load = 0;
                pkgTextStream >> load;
                data.setLoad(load);

                break;
            }

            case 'V': {
                pkgTextStream >> temp; // ':'
                if (temp != ':' || pkgTextStream.status() != QTextStream::Ok) { exitLoop = true; break; }

                float variance = 0;
                pkgTextStream >> variance;
                data.setVariance(variance);

                break;
            }

            case ',':
                // Ignore commas, they separate the parameters
                break;

            case '}':
                success  = true;
                exitLoop = true;
                break;

            default:
                exitLoop = true;
                break;
            }
        }

        // Remove the package from the serialReadData -- we do this by just removing the start character
        this->serialReadData.remove(0,1);

        if (success) {
            KegMeter* meter = this->kegMeters.at(data.getIndex());
            assert(meter != NULL);
            meter->setData(data);
        }
    }
}

void MainWindow::onSelectedSerialPortBytesWritten(qint64 bytes) {
    bytesWritten += bytes;
}

void MainWindow::onEmptyCalibration(const KegMeter& kegMeter) {
    QString serialStr("|Em");
    serialStr += QString::number(kegMeter.getIndex());
    QByteArray serialBytes(serialStr.toStdString().c_str());
    this->serialWrite(serialBytes);
}


void MainWindow::onTrySerialTimer() {
    if (this->selectedSerialPort->isOpen()) {
        return;
    }

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    if (!ports.empty()) {
        // Try to find a port that makes sense...
        foreach (const QSerialPortInfo& port, ports) {
            if (!port.isBusy() && port.isValid()) {
                if (port.manufacturer().contains("Arduino", Qt::CaseInsensitive) ||
                    port.description().contains("Arduino", Qt::CaseInsensitive)) {

                    this->openSerialPort(port);
                    break;
                }
                else if (port.description().contains("AdafruitEZ", Qt::CaseInsensitive)) {

                    this->openSerialPort(port);
                    break;
                }
            }
        }
    }

    // If we still haven't found a good port then try each one...
    if (!this->selectedSerialPort->isOpen()) {
        foreach (const QSerialPortInfo& port, ports) {
            if (port.isBusy() || !port.isValid()) {
                continue;
            }

            this->openSerialPort(port);
        }
    }

    if (!this->selectedSerialPort->isOpen() && !this->trySerialTimer.isActive()) {
        this->trySerialTimer.start(TRY_SERIAL_TIMEOUT_MS);
    }
}

void MainWindow::serialWrite(const QByteArray &writeData) {
    //this->writeData = writeData;

    qint64 bytesWritten = this->selectedSerialPort->write(writeData);
    if (bytesWritten == -1) {
        this->log(tr("Failed to write the data to port %1, error: %2").arg(this->selectedSerialPort->portName()).arg(this->selectedSerialPort->errorString()));
    }
    else if (bytesWritten != writeData.size()) {
       this->log(tr("Failed to write all the data to port %1, error: %2").arg(this->selectedSerialPort->portName()).arg(this->selectedSerialPort->errorString()));
    }
    this->selectedSerialPort->flush();
}


void MainWindow::openSerialPort(const QSerialPortInfo& portInfo) {
    this->selectedSerialPort->setPort(portInfo);
    if (this->selectedSerialPort->open(QIODevice::ReadWrite)) {
        this->log(tr("Connected to %1 @ %2 baud")
                  .arg(this->selectedSerialPort->portName())
                  .arg(this->selectedSerialPort->baudRate())
        );
    }
    else {
        this->log(tr("Failed to connect to serial port %1").arg(portInfo.portName()));
    }
}
