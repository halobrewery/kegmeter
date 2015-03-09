#include "kegmeter.h"
#include "ui_kegmeter.h"

KegMeter::KegMeter(int id, QWidget *parent) : QWidget(parent), ui(new Ui::KegMeter()), id(id) {
    this->ui->setupUi(this);
    this->ui->kegMeterGrpBox->setTitle(tr("Keg Meter ") + QString::number(id, 10));

    this->connect(&this->timer, SIGNAL(timeout()), this, SLOT(onDataTimeout()));
    this->connect(this->ui->emptyCalBtn, SIGNAL(clicked()),
                  this, SLOT(onEmptyCalibrationTriggered()));

    this->timer.setSingleShot(true);

    this->setEnabled(false);
}

KegMeter::~KegMeter() {
    delete this->ui;
    this->ui = NULL;
}

void KegMeter::setData(const KegMeterData& data) {
    assert(data.getId() == this->id);
    bool hasInfo = false;
    float temp = data.getPercent(hasInfo);
    if (hasInfo) {
        this->ui->kegMeterBar->setValue(100*temp);
        this->currData.setPercent(temp);
    }

    hasInfo = false;
    temp = data.getEmptyMass(hasInfo);
    if (hasInfo) {
        this->ui->emptyMassSpinBox->setValue(temp);
        this->currData.setEmptyMass(temp);
    }

    hasInfo = false;
    temp = data.getFullMass(hasInfo);
    if (hasInfo) {
        this->ui->fullKegMassSpinBox->setValue(temp);
        this->currData.setFullMass(temp);
    }

    hasInfo = false;
    temp = data.getLoad(hasInfo);
    if (hasInfo) {
        this->ui->loadSpinBox->setValue(temp);
        this->currData.setLoad(temp);
    }

    hasInfo = false;
    temp = data.getVariance(hasInfo);
    if (hasInfo) {
        this->ui->varianceSpinBox->setValue(temp);
        this->currData.setVariance(temp);
    }

    this->setEnabled(true);
    this->timer.start(DATA_TIMEOUT_MS);
}

void KegMeter::onDataTimeout() {
    this->ui->emptyMassSpinBox->setValue(this->ui->emptyMassSpinBox->minimum());
    this->ui->fullKegMassSpinBox->setValue(this->ui->fullKegMassSpinBox->minimum());
    this->setEnabled(false);
}
