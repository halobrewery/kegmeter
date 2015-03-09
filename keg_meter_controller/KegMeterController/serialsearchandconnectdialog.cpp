#include "serialsearchandconnectdialog.h"
#include "ui_SerialSearchAndConnectDialog.h"
#include "mainwindow.h"

#include <QSerialPortInfo>

#include <cassert>

SerialSearchAndConnectDialog::SerialSearchAndConnectDialog(QSerialPort* serialPort, MainWindow* mainWindow) :
    QDialog(mainWindow),
    ui(new Ui::SerialSearchAndConnectDialog()),
    serialPort(serialPort),
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
    // Populate the ports...
    this->ui->portComboBox->clear();
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    foreach(const QSerialPortInfo& portInfo, portList) {
        this->ui->portComboBox->addItem(portInfo.portName());
    }
    this->ui->portComboBox->setCurrentText(this->serialPort->portName());

    this->ui->baudComboBox->setCurrentText(QString::number(this->serialPort->baudRate()));
}

void SerialSearchAndConnectDialog::onPortComboBoxCurrentIndexChanged(const QString&) {
}

void SerialSearchAndConnectDialog::onBaudComboBoxCurrentIndexChanged(const QString&) {
}

void SerialSearchAndConnectDialog::onReconnectButtonClicked() {
    if (this->ui->manualRadio->isChecked()) {
        this->serialPort->setBaudRate(this->ui->baudComboBox->currentText().toInt());

        auto ports = QSerialPortInfo::availablePorts();
        foreach (const QSerialPortInfo& portInfo, ports) {
            if (portInfo.portName() == this->ui->portComboBox->currentText()) {
                this->serialPort->close();
                this->mainWindow->openSerialPort(portInfo);
            }
        }

        this->mainWindow->log(tr("Failed to find serial port."));
        return;
    }

    // Attempt to auto connect, again...
    this->serialPort->close();
    this->mainWindow->onTrySerialTimer();
}

void SerialSearchAndConnectDialog::autoManualButtonToggled() {
    this->ui->manualGroupBox->setEnabled(this->ui->manualRadio->isChecked());
}
