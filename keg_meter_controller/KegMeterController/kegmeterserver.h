#ifndef KEGMETERCONTROLLER_KEGMETERSERVER_H
#define KEGMETERCONTROLLER_KEGMETERSERVER_H

#include <QTcpServer>

class MainWindow;
class KegMeterConnection;

class KegMeterServer : public QTcpServer {
    Q_OBJECT
public:
    KegMeterServer(MainWindow* mainWindow, QObject* parent = NULL);
    ~KegMeterServer();

signals:
    void newConnection(KegMeterConnection* conn);

private:
    MainWindow* mainWindow;

    void incomingConnection(qintptr socketDescriptor) Q_DECL_OVERRIDE;
};

#endif // KEGMETERCONTROLLER_KEGMETERSERVER_H
