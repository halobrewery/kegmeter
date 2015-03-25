#include "kegmeterserver.h"
#include "kegmeterconnection.h"

#include <cassert>

KegMeterServer::KegMeterServer(MainWindow* mainWindow, QObject* parent) :
    QTcpServer(parent),
    mainWindow(mainWindow) {

    assert(mainWindow != NULL);
    this->listen(QHostAddress::Any);
}

KegMeterServer::~KegMeterServer() {
}

void KegMeterServer::incomingConnection(qintptr socketDescriptor) {
    KegMeterConnection *connection = new KegMeterConnection(this->mainWindow, this);
    connection->setSocketDescriptor(socketDescriptor);
    emit newConnection(connection);
}
