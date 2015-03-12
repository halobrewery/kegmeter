#include "serialsearchandconnectdialog.h"
#include "ui_SerialSearchAndConnectDialog.h"
#include "mainwindow.h"
#include "serialcomm.h"

#include <QSerialPortInfo>

#include <cassert>

SerialSearchAndConnectDialog::SerialSearchAndConnectDialog(SerialComm* comm, MainWindow* mainWindow) :
    QDialog(mainWindow),
    ui(new Ui::SerialSearchAndConnectDialog()),
    comm(comm),
    mainWindow(mainWindow) {

    ui->setupUi(this);

    this->connect(this->ui->portComboBox, SIGNAL(currentIndexChanged(QString)),
                  this, SLOT(onPortComboBoxCurrentIndexChanged(QString)));
    this->connect(this->ui->baudComboBox, SIGNAL(currentIndexChanged(QString)),
                  this, SLOT(onBaudComboBoxCurrentIndexChanged(QString)));
    this->connect(this->ui->reconnectBtn, SIGNAL(clicked()), this, SLOT(onReconnectButtonClicked()));
    this->connect(this->ui->buttonGroup, SIGNAL(buttonToggled(QAbstractButton*,bool)),
                  this, SLOT(autoManualButtonToggled()));

    this->ui->autoRadio->setChecked(true);
}

SerialSearchAndConnectDialog::~SerialSearchAndConnectDialog() {
    delete ui;
}

void SerialSearchAndConnectDialog::showEvent(QShowEvent*) {
    QSerialPort* serialPort = this->comm->getSerialPort();
    assert(serialPort != NULL);

    // Populate the ports...
    this->ui->portComboBox->clear();
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    foreach(const QSerialPortInfo& portInfo, portList) {
        this->ui->portComboBox->addItem(portInfo.portName());
    }
    this->ui->portComboBox->setCurrentText(serialPort->portName());

    this->ui->baudComboBox->setCurrentText(QString::number(serialPort->baudRate()));
}

void SerialSearchAndConnectDialog::onPortComboBoxCurrentIndexChanged(const QString&) {
}

void SerialSearchAndConnectDialog::onBaudComboBoxCurrentIndexChanged(const QString&) {
}

void SerialSearchAndConnectDialog::onReconnectButtonClicked() {
    QSerialPort* serialPort = this->comm->getSerialPort();
    if (this->ui->manualRadio->isChecked()) {
        serialPort->setBaudRate(this->ui->baudComboBox->currentText().toInt());

        auto ports = QSerialPortInfo::availablePorts();
        foreach (const QSerialPortInfo& portInfo, ports) {
            if (portInfo.portName() == this->ui->portComboBox->currentText()) {
                serialPort->close();
                this->comm->openSerialPort(portInfo);
            }
        }

        this->mainWindow->log(tr("Failed to find serial port."));
        return;
    }

    // Attempt to auto connect, again...
    serialPort->close();
    this->comm->onTrySerialTimer();
}

void SerialSearchAndConnectDialog::autoManualButtonToggled() {
    this->ui->manualGroupBox->setEnabled(this->ui->manualRadio->isChecked());
}
