#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QtSerialPort/QSerialPort>

namespace Ui {
class PreferencesDialog;
}

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    PreferencesDialog(QSerialPort* serialPort, QWidget *parent = 0);
    ~PreferencesDialog();

    void showEvent(QShowEvent* event);

private slots:
    void onPortComboBoxCurrentIndexChanged(const QString& selectedText);
    void onBaudComboBoxCurrentIndexChanged(const QString& selectedText);

private:
    Ui::PreferencesDialog *ui;
    QSerialPort* serialPort;
};

#endif // PREFERENCESDIALOG_H
