#ifndef KEGMETERCONTROLLER_KEGMETER_H
#define KEGMETERCONTROLLER_KEGMETER_H

#include <QWidget>
#include <QTimer>

namespace Ui {
class KegMeter;
}

class MainWindow;
class AbstractComm;
class CalibrateKegMeterDialog;

class KegMeter : public QWidget {
    Q_OBJECT
public:
    explicit KegMeter(int id, AbstractComm* comm, MainWindow* parent);
    ~KegMeter();

    int getId() const { return this->id; }
    int getIndex() const { return this->id-1; }

    bool isEmptyCalComplete() const { return this->emptyCalComplete; }
    bool isNonEmptyCalComplete() const { return this->nonEmptyCalComplete; }

    void updateLoadMeasurement(float sensorLoadValue);

    void outputSync() { this->outputSync(this->currState); }

    void performEmptyCalibration();
    void performNonEmptyCalibration(float actualMass);

signals:
    void finishedEmptyCalibration();
    void finishedNonEmptyCalibration();

private slots:
    void onCalibrate();
    void onKegTypeChanged();
    void onReset();
    void onDataTimeout();


private:
    AbstractComm* comm; // Not owned by this
    MainWindow* mainWindow;
    CalibrateKegMeterDialog* calDialog;

    Ui::KegMeter* ui;
    int id;

    static const int DATA_TIMEOUT_MS = 10000;
    QTimer timer;

    enum KegType { Corny19LKeg, Sankey50LKeg } currKegType;

    enum State {
      NonEmptyCalibration, // Calibration of the load sensor for this meter when a known mass has been placed on it
      EmptyCalibration,    // Calibration of the load sensor for this meter when nothing has been placed on it
      Empty,               // State to rest in when the keg is empty or there is no keg on the sensor for this meter
      Calibrating,         // A keg (or something with mass) has been detected on the sensor and it needs to calibrate for it
      Measuring,           // This is the "typical" state for showing the current status of the keg as people drink from it 100%->0% on the meter
      JustBecameEmpty      // The keg JUST became empty
    } currState;

    // Stateful members: keep track of information in various states
    int dataCounter;
    float lastPercentAmt;

    bool emptyCalComplete;
    bool nonEmptyCalComplete;

    // Mass-ful calibration values
    float emptyCalSensorValue;
    float nonEmptyCalMass;
    float nonEmptyCalSensorValue;

    // Filter window members
    static const int LOAD_WINDOW_SIZE = 30;
    QList<float> loadWindow;
    float loadWindowSum;

    void outputSync(State prevState);

    void setState(State newState);
    void setKegType(KegType kegType);

    void setCalEmptySensorValue(float emptyAmt);

    void fillLoadWindow(float value);
    void putInLoadWindow(float value);

    float getLoadWindowMean() const { return this->loadWindowSum / static_cast<float>(this->loadWindow.size()); }
    float getLoadWindowVariance() const;

    float getEmptyKegMass() const;
    float getAvgFullKegMass() const;


    float calcCalibratedMass(float sensorValue);
    float calcCurrMeanPercentage() const;

    void outputPercent();
    void outputRoutine(char routineType);

    void readFromSettings();
    void writeToSettings();
};


#endif // KEGMETERCONTROLLER_KEGMETER_H
