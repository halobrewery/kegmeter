#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "kegmeter.h"
#include "kegmeterdata.h"
#include "serialcomm.h"
#include "appsettings.h"

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
#include <QSettings>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow()) {

    this->ui->setupUi(this);

    this->comm = new SerialComm(this);

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
                KegMeter* meter = new KegMeter(j+1, this->comm, this);
                this->kegMeters.push_back(meter);
                vLayout->addWidget(meter);
            }
        }
        mainLayout->addLayout(vLayout);
    }

    this->ui->centralWidget->setLayout(mainLayout);
    this->setWindowTitle("Halo Keg Meter Control Panel");

    this->connect(this->comm, SIGNAL(kegMeterDataAvailable(const KegMeterData&)), this, SLOT(onKegMeterData(const KegMeterData&)));
    this->connect(this->comm, SIGNAL(commClosed()), this, SLOT(onCommClosed()));

    this->connect(this->ui->serialInfoAction, SIGNAL(triggered()), this, SLOT(onSerialInfoActionTriggered()));
    this->connect(this->ui->serialSearchAndConnectAction, SIGNAL(triggered()),
                  this, SLOT(onSerialSearchAndConnectDialogActionTriggered()));

    this->serialInfoDialog = new QDialog(this);
    this->serialInfoDialog->setFixedSize(375, 400);
    this->serialInfoDialog->setWindowTitle("Serial Port Info");
}

MainWindow::~MainWindow() {


    this->writeKegMeterDataToSettings();

    delete this->serialInfoDialog;
    this->serialInfoDialog = NULL;

    delete this->ui;
    this->ui = NULL;

    delete this->comm;
    this->comm = NULL;
}

void MainWindow::log(const QString& logStr, bool newLine) {
    this->ui->appLogTextEdit->insertPlainText(logStr + (newLine ? tr("\n") : tr("")));
    this->ui->appLogTextEdit->verticalScrollBar()->setValue(this->ui->appLogTextEdit->verticalScrollBar()->maximum());
}

void MainWindow::commLog(const QString& logStr) {
    this->ui->serialLogTextEdit->insertPlainText(logStr);
    this->ui->serialLogTextEdit->verticalScrollBar()->setValue(this->ui->serialLogTextEdit->verticalScrollBar()->maximum());   
}

void MainWindow::onCommClosed() {
    // Disable all of the meter GUIs
    foreach (KegMeter* meter, this->kegMeters) {
        meter->setEnabled(false);
    }
}

void MainWindow::onKegMeterData(const KegMeterData& data) {
    KegMeter* meter = this->kegMeters.at(data.getIndex());
    assert(meter != NULL);
    meter->setData(data);
    this->writeKegMeterDataToSettings();
}

void MainWindow::onSerialSearchAndConnectDialogActionTriggered() {
    this->comm->executeSettingsDialog();
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

void MainWindow::writeKegMeterDataToSettings() const {
    QSettings settings;
    bool temp;
    for (auto iter = this->kegMeters.cbegin(); iter != this->kegMeters.cend(); ++iter) {
        const KegMeter* meter = *iter;

        // Check to see if there's any actual data... if there isn't then don't overwrite it!
        QVariant existingData = settings.value(QString(AppSettings::KEG_DATA_KEY) + QString("/") + QString::number(meter->getIndex()));
        if (!existingData.isNull()) {
            KegMeterData existingMeterData = existingData.value<KegMeterData>();
            if ((meter->getData().buildUpdateSerialStr(meter->getIndex()).isEmpty() ||
                 meter->getData().getPercent(temp) <= 0.0) &&
                !existingMeterData.buildUpdateSerialStr(meter->getIndex()).isEmpty()) {
                continue;
            }
        }

        settings.setValue(QString(AppSettings::KEG_DATA_KEY) + QString("/") + QString::number(meter->getIndex()), QVariant::fromValue(meter->getData()));
    }
}
