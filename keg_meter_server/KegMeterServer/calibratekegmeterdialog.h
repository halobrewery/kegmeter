#ifndef CALIBRATEKEGMETERDIALOG_H
#define CALIBRATEKEGMETERDIALOG_H

#include <QDialog>

namespace Ui {
class CalibrateKegMeterDialog;
}

class KegMeter;

class CalibrateKegMeterDialog : public QDialog {
    Q_OBJECT

public:
    explicit CalibrateKegMeterDialog(KegMeter *kegMeter);
    ~CalibrateKegMeterDialog();

private slots:
    void onEmptyCalibrate();
    void onNonEmptyCalibrate();
    void onFinishedEmptyCalibration();
    void onFinishedNonEmptyCalibration();

private:
    KegMeter* kegMeter;
    Ui::CalibrateKegMeterDialog *ui;

    void updateEmptyCalibrateView();
    void updateNonEmptyCalibrateView();
};

#endif // CALIBRATEKEGMETERDIALOG_H
