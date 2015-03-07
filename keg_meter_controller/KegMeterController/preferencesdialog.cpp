#include "preferencesdialog.h"
#include "ui_preferencesdialog.h"

#include <QSerialPortInfo>

PreferencesDialog::PreferencesDialog(QSerialPort* serialPort, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog()),
    serialPort(serialPort) {

    ui->setupUi(this);

    this->connect(this->ui->portComboBox, SIGNAL(currentIndexChanged(QString)),
                  this, SLOT(onPortComboBoxCurrentIndexChanged(QString)));
    this->connect(this->ui->baudComboBox, SIGNAL(currentIndexChanged(QString)),
                  this, SLOT(onBaudComboBoxCurrentIndexChanged(QString)));
}

PreferencesDialog::~PreferencesDialog() {
    delete ui;
}

void PreferencesDialog::showEvent(QShowEvent*) {
    // Populate the ports...
    this->ui->portComboBox->clear();
    QList<QSerialPortInfo> portList = QSerialPortInfo::availablePorts();
    foreach(const QSerialPortInfo& portInfo, portList) {
        this->ui->portComboBox->addItem(portInfo.portName());
    }
    this->ui->portComboBox->setCurrentText(this->serialPort->portName());

    this->ui->baudComboBox->setCurrentText(QString::number(this->serialPort->baudRate()));
}

void PreferencesDialog::onPortComboBoxCurrentIndexChanged(const QString& selectedText) {
    if (this->serialPort->isOpen()) {
        this->serialPort->close();
    }

    this->serialPort->setPortName(selectedText);

    this->serialPort->open(QIODevice::ReadWrite);
}

void PreferencesDialog::onBaudComboBoxCurrentIndexChanged(const QString& selectedText) {
    if (this->serialPort->isOpen()) {
        this->serialPort->close();
    }

    this->serialPort->setBaudRate(selectedText.toInt());

    this->serialPort->open(QIODevice::ReadWrite);
}
