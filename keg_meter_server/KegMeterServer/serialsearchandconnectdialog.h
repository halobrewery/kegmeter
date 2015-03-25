#ifndef KEGMETERCONTROLLER_SERIALSEARCHANDCONNECTDIALOG_H
#define KEGMETERCONTROLLER_SERIALSEARCHANDCONNECTDIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include <QtSerialPort/QSerialPort>

class MainWindow;
class SerialComm;

namespace Ui {
class SerialSearchAndConnectDialog;
}

class SerialSearchAndConnectDialog : public QDialog {
    Q_OBJECT

public:
    SerialSearchAndConnectDialog(SerialComm* comm, MainWindow* mainWindow);
    ~SerialSearchAndConnectDialog();

    void showEvent(QShowEvent* event);

private slots:
    void onPortComboBoxCurrentIndexChanged(const QString& selectedText);
    void onBaudComboBoxCurrentIndexChanged(const QString& selectedText);
    void onReconnectButtonClicked();
    void autoManualButtonToggled();

private:
    Ui::SerialSearchAndConnectDialog *ui;
    SerialComm* comm;
    MainWindow* mainWindow;
};

#endif // KEGMETERCONTROLLER_SERIALSEARCHANDCONNECTDIALOG_H
