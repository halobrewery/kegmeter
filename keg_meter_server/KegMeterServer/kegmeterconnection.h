#ifndef KEGMETERCONNECTION_H
#define KEGMETERCONNECTION_H

#include <QTcpSocket>
#include <QHostAddress>
#include <QString>
#include <QTime>
#include <QTimer>

class MainWindow;

class KegMeterConnection : public QTcpSocket {
    Q_OBJECT
public:
    KegMeterConnection(MainWindow* mainWindow, QObject* parent = NULL);
    ~KegMeterConnection();

    QString name() const;
    bool sendMessage(const QString &message);

private slots:
    void processReadyRead();
    void sendPing();
    void onConnected();

private:
    static const int RESPONSE_TIMEOUT_MS = 10000;
    static const int MAX_BUFFER_SIZE = 1024000;
    static const char PKG_START_TOKEN;
    static const char PKG_END_TOKEN;

    MainWindow* mainWindow;

    QTimer pingTimer;
    QTime timeSinceLastResponse;
    QByteArray buffer;
};

#endif // KEGMETERCONNECTION_H
