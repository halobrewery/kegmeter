#ifndef KEGMETERCONTROLLER_KEGMETER_H
#define KEGMETERCONTROLLER_KEGMETER_H

#include <QWidget>
#include <QTimer>

#include "kegmeterdata.h"

namespace Ui {
class KegMeter;
}

class AbstractComm;

class KegMeter : public QWidget {
    Q_OBJECT
public:
    explicit KegMeter(int id, AbstractComm* comm, QWidget* parent);
    ~KegMeter();

    int getId() const { return this->id; }
    int getIndex() const { return this->id-1; }

    void setData(const KegMeterData& data);
    const KegMeterData& getData() const { return this->currData; }

private slots:
    void onEmptyCalibration();
    void onReset();
    void onDataTimeout();

private:
    AbstractComm* comm; // Not owned by this

    Ui::KegMeter* ui;
    int id;
    KegMeterData currData;

    static const int DATA_TIMEOUT_MS = 10000;
    QTimer timer;
};

#endif // KEGMETERCONTROLLER_KEGMETER_H
