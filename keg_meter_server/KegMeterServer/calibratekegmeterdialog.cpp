#include "calibratekegmeterdialog.h"
#include "ui_calibratekegmeterdialog.h"
#include "kegmeter.h"

CalibrateKegMeterDialog::CalibrateKegMeterDialog(KegMeter* kegMeter) :
    QDialog(kegMeter),
    kegMeter(kegMeter),
    ui(new Ui::CalibrateKegMeterDialog) {

    this->ui->setupUi(this);

    QObject::connect(this->ui->emptyCalBtn, SIGNAL(clicked()), this, SLOT(onEmptyCalibrate()));
    QObject::connect(this->ui->nonEmptyCalBtn, SIGNAL(clicked()), this, SLOT(onNonEmptyCalibrate()));

    QObject::connect(this->kegMeter, SIGNAL(finishedEmptyCalibration()), this, SLOT(onFinishedEmptyCalibration()));
    QObject::connect(this->kegMeter, SIGNAL(finishedNonEmptyCalibration()), this, SLOT(onFinishedNonEmptyCalibration()));
}

CalibrateKegMeterDialog::~CalibrateKegMeterDialog() {
    delete this->ui;
    this->ui = NULL;
}

void CalibrateKegMeterDialog::onEmptyCalibrate() {
    this->ui->emptyCalStatusLbl->setText("Calibrating...");
    this->kegMeter->performEmptyCalibration();
}

void CalibrateKegMeterDialog::onNonEmptyCalibrate() {
    this->ui->nonEmptyCalStatusLbl->setText("Calibrating...");
    this->kegMeter->performNonEmptyCalibration(this->ui->nonEmptyCalSpinBox->value());
}

void CalibrateKegMeterDialog::onFinishedEmptyCalibration() {
    this->updateEmptyCalibrateView();
}

void CalibrateKegMeterDialog::onFinishedNonEmptyCalibration() {
    this->updateNonEmptyCalibrateView();
}

void CalibrateKegMeterDialog::updateEmptyCalibrateView() {
    if (this->kegMeter->isEmptyCalComplete()) {
        this->ui->emptyCalStatusLbl->setText("Calibrated!");
    }
    else {
        this->ui->emptyCalStatusLbl->setText("Not Calibrated");
    }
}

void CalibrateKegMeterDialog::updateNonEmptyCalibrateView() {
    if (this->kegMeter->isNonEmptyCalComplete()) {
        this->ui->nonEmptyCalStatusLbl->setText("Calibrated!");
    }
    else {
        this->ui->nonEmptyCalStatusLbl->setText("Not Calibrated");
    }
}
