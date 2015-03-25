#include "kegmeter.h"
#include "ui_kegmeter.h"
#include "abstractcomm.h"
#include "mainwindow.h"
#include "appsettings.h"
#include "calibratekegmeterdialog.h"

#include <QSettings>
#include <QMessageBox>

static const float MIN_LOAD_WINDOW_VARIANCE_CALIBRATION = 0.05;
static const float MIN_TRUSTWORTHY_VARIANCE_WHILE_MEASURING = 0.5;

static const float AVG_EMPTY_CORNY_KEG_MASS_KG = 4.0;
static const float AVG_FULL_CORNY_KEG_MASS_KG  = (18 * 1.005) + AVG_EMPTY_CORNY_KEG_MASS_KG;

static const float AVG_EMPTY_50L_KEG_MASS_KG = 13.5;
static const float AVG_FULL_50L_KEG_MASS_KG  = (48 * 1.005) + AVG_EMPTY_50L_KEG_MASS_KG;

static const float MAX_FULL_CORNY_KEG_MASS_KG = (19 * 1.035) + AVG_EMPTY_CORNY_KEG_MASS_KG;
static const float MAX_FULL_50L_KEG_MASS_KG   = (50 * 1.035) + AVG_EMPTY_50L_KEG_MASS_KG;

// This needs to be a bit lighter than the lightest empty keg in use
static const float EMPTY_TO_CALIBRATING_MASS = (AVG_EMPTY_CORNY_KEG_MASS_KG + 5);

static float linearInterpolation(float x, float x0, float x1, float y0, float y1) {
  return (y0 + (y1-y0)*(x-x0)/(x1-x0));
}

KegMeter::KegMeter(int id, AbstractComm* comm, MainWindow* parent) :
    QWidget(parent),
    comm(comm),
    mainWindow(parent),
    calDialog(NULL),
    ui(new Ui::KegMeter()),
    id(id),
    currKegType(Corny19LKeg),
    currState(Empty),
    dataCounter(0),
    lastPercentAmt(0),
    loadWindowSum(0),
    emptyCalComplete(false),
    nonEmptyCalComplete(false) {

    assert(comm != NULL);

    this->ui->setupUi(this);
    this->ui->kegMeterGrpBox->setTitle(tr("Keg Meter ") + QString::number(id, 10));

    // Populate the keg type combo box
    this->ui->kegTypeComboBox->addItem(tr("19L Cornelius Keg"), Corny19LKeg);
    this->ui->kegTypeComboBox->addItem(tr("50L Sankey Keg"), Sankey50LKeg);

    this->calDialog = new CalibrateKegMeterDialog(this);

    this->readFromSettings();

    QObject::connect(this->ui->kegTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(onKegTypeChanged()));
    QObject::connect(this->ui->calibrateBtn, SIGNAL(clicked()), this, SLOT(onCalibrate()));
    QObject::connect(&this->timer, SIGNAL(timeout()), this, SLOT(onDataTimeout()));
    QObject::connect(this->ui->resetBtn, SIGNAL(clicked()), this, SLOT(onReset()));

    this->timer.setSingleShot(true);

    this->setEnabled(false);
}

KegMeter::~KegMeter() {
    this->writeToSettings();

    delete this->ui;
    this->ui = NULL;
}

void KegMeter::updateLoadMeasurement(float sensorLoadValue) {
    this->setEnabled(true);

    this->putInLoadWindow(this->calcCalibratedMass(sensorLoadValue));

    switch (this->currState) {
    case NonEmptyCalibration: {
        this->dataCounter++;

        // We fill the load window and find the average sensor value
        float variance = this->getLoadWindowVariance();
        if (variance <= MIN_LOAD_WINDOW_VARIANCE_CALIBRATION && this->dataCounter >= LOAD_WINDOW_SIZE) {
            this->nonEmptyCalSensorValue = this->getLoadWindowMean();
            this->fillLoadWindow(this->nonEmptyCalMass);

            this->mainWindow->log(QString("Keg meter %1: Non-Empty Calibration Complete. Calibrated Amount: %2 -> %3")
                                  .arg(this->id)
                                  .arg(this->nonEmptyCalSensorValue)
                                  .arg(this->nonEmptyCalMass));

            this->nonEmptyCalComplete = true;
            emit finishedNonEmptyCalibration();
            this->setState(Empty);
        }
        break;
    }

    case EmptyCalibration: {
        this->dataCounter++;

        // We fill the load window and find the average sensor value
        float variance = this->getLoadWindowVariance();
        if (variance <= MIN_LOAD_WINDOW_VARIANCE_CALIBRATION && this->dataCounter >= LOAD_WINDOW_SIZE) {

            this->setCalEmptySensorValue(this->getLoadWindowMean());
            this->fillLoadWindow(0);

            this->mainWindow->log(QString("Keg meter %1: Empty Calibration Complete. Calibrated Empty Amount: %2 -> 0")
                                  .arg(this->id)
                                  .arg(this->emptyCalSensorValue));

            this->emptyCalComplete = true;
            emit finishedEmptyCalibration();
            this->setState(Empty);
        }
        break;
    }

    case Empty: {
        // Waiting until someone puts a new full/partially-full keg on the sensor...
        float variance = this->getLoadWindowVariance();
        float mean = this->getLoadWindowMean();
        if (variance <= MIN_LOAD_WINDOW_VARIANCE_CALIBRATION && mean >= EMPTY_TO_CALIBRATING_MASS) {
            this->setState(Calibrating);
        }
        break;
    }

    case Calibrating: {
        this->dataCounter++;

        float variance = this->getLoadWindowVariance();
        float mean = this->getLoadWindowMean();

        // Check to see if the load goes back below the "empty" threshold
        if (mean < EMPTY_TO_CALIBRATING_MASS) {
            this->setState(Empty);
        }
        else {
            // Wait until the variance goes below a certain threshold and wait until we've filled the
            // load window enough...
            static const int CALIBRATE_COLLECTION_SIZE = LOAD_WINDOW_SIZE;

            float percentCalibrated =
                    linearInterpolation(std::min<float>(CALIBRATE_COLLECTION_SIZE, this->dataCounter),
                                        0, CALIBRATE_COLLECTION_SIZE, 0.0, 1.0);
            assert(percentCalibrated >= 0 && percentCalibrated <= 1);

            // We want the calibrating display to show a little bit of something no matter what
            percentCalibrated = std::max<float>(0.1, percentCalibrated);

            this->lastPercentAmt = percentCalibrated;
            this->outputPercent();

            if (variance <= MIN_LOAD_WINDOW_VARIANCE_CALIBRATION && percentCalibrated >= 1.0) {
                this->lastPercentAmt = 1.0;
                this->setState(Measuring);
            }
        }
        break;
    }

    case Measuring: {

        float variance = this->getLoadWindowVariance();
        if (variance <= MIN_TRUSTWORTHY_VARIANCE_WHILE_MEASURING) {

            // The meter is  set by the current load amount based on a linear interpolation between
            // the initial calibrated full load and a reasonable "zero" load
            float currPercentAmt = this->calcCurrMeanPercentage();

            // Don't measure backwards, the meter can only wind down, not up
            if (currPercentAmt < this->lastPercentAmt) {
                this->lastPercentAmt = currPercentAmt;
                this->outputPercent();
            }
        }

        if (this->lastPercentAmt < 0.01) {
            this->setState(JustBecameEmpty);
        }
        break;
    }

    case JustBecameEmpty:
        this->dataCounter++;
        if (this->dataCounter >= LOAD_WINDOW_SIZE) {
            this->setState(Empty);
        }
        break;

    default:
        break;
    }
}

void KegMeter::outputSync(State prevState) {
    this->outputPercent();

    switch (this->currState) {

    case NonEmptyCalibration:
        this->outputRoutine('O');
        break;

    case EmptyCalibration:
        this->outputRoutine('O');
        break;

    case Empty:
        if (prevState != JustBecameEmpty) {
            this->outputRoutine('O');
        }
        break;

    case Calibrating:
        this->outputRoutine('C');
        break;

    case Measuring:
        if (prevState == Calibrating) {
            this->outputRoutine('F');
        }
        else {
            this->outputRoutine('M');
        }
        break;

    case JustBecameEmpty:
        this->outputRoutine('E');
        break;

    default:
        assert(false);
        break;
    }
}

void KegMeter::performEmptyCalibration() {
    this->setState(EmptyCalibration);
}

void KegMeter::performNonEmptyCalibration(float actualMass) {
    this->nonEmptyCalComplete = false;
    this->nonEmptyCalMass = actualMass;
    this->setState(NonEmptyCalibration);
}

void KegMeter::onCalibrate() {
    this->calDialog->exec();
}

void KegMeter::onKegTypeChanged() {
    bool success = false;
    int kegTypeInt = this->ui->kegTypeComboBox->currentData().toInt(&success);
    assert(success);
    this->setKegType(static_cast<KegType>(kegTypeInt));
}

void KegMeter::onReset() {
    // Ask if the user REALLY wants to do this
    int result = QMessageBox::question(this, "Reset Keg Meter", "Are you sure you want to reset? Resetting will clear all calibration information.", QMessageBox::Cancel, QMessageBox::Ok);

    if (result == QMessageBox::Ok) {
        this->emptyCalComplete = false;
        this->nonEmptyCalComplete = false;
        this->setState(Empty);
    }
}

void KegMeter::onDataTimeout() {
    this->ui->fullKegMassSpinBox->setValue(this->ui->fullKegMassSpinBox->minimum());
    this->setEnabled(false);
}

void KegMeter::setState(State newState) {

    switch (newState) {

    case NonEmptyCalibration:
        this->mainWindow->log(QString("Keg Meter %1: Entering Non-Empty Calibration State").arg(this->id));
        this->dataCounter = 0;
        this->lastPercentAmt = 0;
        break;

    case EmptyCalibration:
        this->mainWindow->log(QString("Keg Meter %1: Entering Empty Calibration State").arg(this->id));
        this->dataCounter = 0;
        this->lastPercentAmt = 0;
        break;

    case Empty:
        this->mainWindow->log(QString("Keg Meter %1: Entering Empty State").arg(this->id));
        this->dataCounter = 0;
        this->lastPercentAmt = 0;
        break;

    case Calibrating:
        this->mainWindow->log(QString("Keg Meter %1: Entering Calibrating State").arg(this->id));
        this->dataCounter = 0;
        break;

    case Measuring:
        this->mainWindow->log(QString("Keg Meter %1: Entering Measuring State").arg(this->id));
        this->dataCounter = 0;
        break;

    case JustBecameEmpty:
        this->mainWindow->log(QString("Keg Meter %1: Entering Just Became Empty State").arg(this->id));
        this->dataCounter = 0;
        this->lastPercentAmt = 0;
        break;

    default:
        assert(false);
        return;
    }

    State prevState = this->currState;
    this->currState = newState;
    this->outputSync(prevState);
}

void KegMeter::setKegType(KegType kegType) {
    this->currKegType = kegType;

    this->ui->kegTypeComboBox->blockSignals(true);
    int idx = this->ui->kegTypeComboBox->findData(kegType);
    assert(idx >= 0);
    this->ui->kegTypeComboBox->setCurrentIndex(idx);
    this->ui->kegTypeComboBox->blockSignals(false);

    this->ui->fullKegMassSpinBox->setValue(this->getAvgFullKegMass());

    // Re-calculate the last percentage amount
    this->lastPercentAmt = this->calcCurrMeanPercentage();
}

void KegMeter::setCalEmptySensorValue(float emptyAmt) {
    this->emptyCalSensorValue = emptyAmt;
}

void KegMeter::fillLoadWindow(float value) {
    this->loadWindow.clear();
    for (int i = 0; i < LOAD_WINDOW_SIZE; i++) {
        this->loadWindow.push_back(value);
    }
    this->loadWindowSum = value * LOAD_WINDOW_SIZE;

    this->ui->loadSpinBox->setValue(value);
    this->ui->varianceSpinBox->setValue(this->getLoadWindowVariance());
}

void KegMeter::putInLoadWindow(float value) {
    if (this->loadWindow.size() == LOAD_WINDOW_SIZE) {
        this->loadWindowSum -= this->loadWindow.front();
        this->loadWindow.pop_front();

        this->loadWindowSum += value;
        this->loadWindow.push_back(value);
    }
    else {
        this->fillLoadWindow(value);
    }

    this->ui->loadSpinBox->setValue(this->getLoadWindowMean());
    this->ui->varianceSpinBox->setValue(this->getLoadWindowVariance());
}

float KegMeter::getLoadWindowVariance() const {
    float currMean = this->getLoadWindowMean();
    float variance = 0;
    foreach (float value, this->loadWindow) {
        float diff = (value - currMean);
        variance += diff*diff;
    }
    variance /= static_cast<float>(this->loadWindow.size());
    return variance;
}

float KegMeter::getEmptyKegMass() const {
    switch (this->currKegType) {

    case Corny19LKeg:
        return AVG_EMPTY_CORNY_KEG_MASS_KG;
    case Sankey50LKeg:
        return AVG_EMPTY_50L_KEG_MASS_KG;

    default:
        assert(false);
        return AVG_EMPTY_50L_KEG_MASS_KG;
    }
}

float KegMeter::getAvgFullKegMass() const {
    switch (this->currKegType) {

    case Corny19LKeg:
        return AVG_FULL_CORNY_KEG_MASS_KG;
    case Sankey50LKeg:
        return AVG_FULL_50L_KEG_MASS_KG;

    default:
        assert(false);
        return AVG_FULL_50L_KEG_MASS_KG;
    }
}

float KegMeter::calcCurrMeanPercentage() const {
    return std::max<float>(0.0, std::min<float>(1.0,
        linearInterpolation(this->getLoadWindowMean(), this->getEmptyKegMass(),
                            this->getAvgFullKegMass(), 0.0, 1.0)));
}

float KegMeter::calcCalibratedMass(float sensorValue) {
    // Don't adjust the sensor value when we're calibrating!
    if (this->currState == NonEmptyCalibration ||
        this->currState == EmptyCalibration) {

        return sensorValue;
    }

    float calValue = sensorValue;
    if (this->emptyCalComplete) {
        if (this->nonEmptyCalComplete) {
            float denom = (this->nonEmptyCalSensorValue - this->emptyCalSensorValue);
            if (denom <= 0) {
                return calValue;
            }
            calValue = (sensorValue - this->emptyCalSensorValue) *
                    (this->nonEmptyCalMass - 0) / denom + 0;
        }
        else {
            calValue -= this->emptyCalSensorValue;
        }
    }

    return calValue;
}

void KegMeter::outputPercent() {
    QString serialStr = QString("[%1 %2 %3]")
            .arg(this->getIndex(), 2, 10, QChar('0'))
            .arg(QChar('P'))
            .arg(this->lastPercentAmt, 4, 'f', 2, QChar('0'));

    this->comm->writeString(serialStr);
    this->writeToSettings();
}

void KegMeter::outputRoutine(char routineType) {
    QString serialStr = QString("[%1 %2 %3]")
            .arg(this->getIndex(), 2, 10, QChar('0'))
            .arg(QChar('R'))
            .arg(QChar(routineType));

    this->comm->writeString(serialStr);
    this->writeToSettings();
}

void KegMeter::readFromSettings() {
    QSettings settings;

    KegType kegType = static_cast<KegType>(
                settings.value(AppSettings::buildKegMeterKey(this->getIndex(), AppSettings::KEG_METER_KEGTYPE), Corny19LKeg).toInt());
    this->lastPercentAmt = settings.value(
                AppSettings::buildKegMeterKey(this->getIndex(), AppSettings::KEG_METER_PERCENT), 0.0).toFloat();

    this->setCalEmptySensorValue(settings.value(
                AppSettings::buildKegMeterKey(this->getIndex(), AppSettings::KEG_METER_CAL_EMPTY_SENSOR_VAL), 0.0).toFloat());
    this->nonEmptyCalSensorValue = settings.value(
                AppSettings::buildKegMeterKey(this->getIndex(), AppSettings::KEG_METER_CAL_NONEMPTY_SENSOR_VAL), 0.0).toFloat();
    this->nonEmptyCalMass = settings.value(
                AppSettings::buildKegMeterKey(this->getIndex(), AppSettings::KEG_METER_CAL_NONEMPTY_MASS_VAL), 0.0).toFloat();

    this->emptyCalComplete = !settings.value(
                AppSettings::buildKegMeterKey(this->getIndex(), AppSettings::KEG_METER_CAL_EMPTY_SENSOR_VAL)).isNull();
    this->nonEmptyCalComplete = !settings.value(
                AppSettings::buildKegMeterKey(this->getIndex(), AppSettings::KEG_METER_CAL_NONEMPTY_SENSOR_VAL)).isNull();

    this->setKegType(kegType);
    if (this->lastPercentAmt <= 0) {
        this->lastPercentAmt = 0;
        this->setState(Empty);
    }
    else {
        this->setState(Measuring);
    }
}

void KegMeter::writeToSettings() {
    QSettings settings;

    settings.setValue(AppSettings::buildKegMeterKey(this->getIndex(), AppSettings::KEG_METER_KEGTYPE), this->currKegType);
    settings.setValue(AppSettings::buildKegMeterKey(this->getIndex(), AppSettings::KEG_METER_PERCENT), this->lastPercentAmt);

    if (this->emptyCalComplete) {
        settings.setValue(AppSettings::buildKegMeterKey(this->getIndex(), AppSettings::KEG_METER_CAL_EMPTY_SENSOR_VAL), this->emptyCalSensorValue);
    }
    if (this->nonEmptyCalComplete) {
        settings.setValue(AppSettings::buildKegMeterKey(this->getIndex(), AppSettings::KEG_METER_CAL_NONEMPTY_SENSOR_VAL), this->nonEmptyCalSensorValue);
        settings.setValue(AppSettings::buildKegMeterKey(this->getIndex(), AppSettings::KEG_METER_CAL_NONEMPTY_MASS_VAL), this->nonEmptyCalMass);
    }
}
