#ifndef KEGMETERCONTROLLER_MAINWINDOW_H
#define KEGMETERCONTROLLER_MAINWINDOW_H

#include <QMainWindow>

class AbstractComm;
class KegMeter;
class KegMeterData;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    int getNumKegMeters() const { return this->kegMeters.size(); }

    void log(const QString& logStr, bool newLine = true);
    void commLog(const QString& logStr);

    void writeKegMeterDataToSettings() const;

public slots:
    void onKegMeterData(const KegMeterData& data);
    void onCommClosed();

private slots:
    void onSerialSearchAndConnectDialogActionTriggered();
    void onSerialInfoActionTriggered();

private:
    Ui::MainWindow* ui;

    AbstractComm* comm;
    QDialog* serialInfoDialog;

    static const int NUM_KEG_METERS = 8;
    QList<KegMeter*> kegMeters;
};

#endif // KEGMETERCONTROLLER_MAINWINDOW_H
