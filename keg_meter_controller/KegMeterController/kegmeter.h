#ifndef KEGMETERCONTROLLER_KEGMETER_H
#define KEGMETERCONTROLLER_KEGMETER_H

#include <QWidget>
#include <QTimer>

#include "kegmeterdata.h"

namespace Ui {
class KegMeter;
}

class KegMeter : public QWidget {
    Q_OBJECT
public:
    explicit KegMeter(int id, QWidget *parent = NULL);
    ~KegMeter();

    int getId() const { return this->id; }
    int getIndex() const { return this->id-1; }

    void setData(const KegMeterData& data);
    const KegMeterData& getData() const { return this->currData; }

signals:
    void doEmptyCalibration(const KegMeter& kegMeter);

private slots:
    void onEmptyCalibrationTriggered() { emit doEmptyCalibration(*this); }
    void onDataTimeout();

private:
    Ui::KegMeter* ui;
    int id;
    KegMeterData currData;

    static const int DATA_TIMEOUT_MS = 10000;
    QTimer timer;
};

#endif // KEGMETERCONTROLLER_KEGMETER_H
