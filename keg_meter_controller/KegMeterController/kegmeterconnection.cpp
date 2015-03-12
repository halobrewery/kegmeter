#include "kegmeterconnection.h"
#include "mainwindow.h"

#include <cassert>

const char KegMeterConnection::PKG_START_TOKEN = '[';
const char KegMeterConnection::PKG_END_TOKEN   = ']';

KegMeterConnection::KegMeterConnection(MainWindow* mainWindow, QObject* parent) :
    QTcpSocket(parent),
    mainWindow(mainWindow) {

    assert(mainWindow != NULL);

    QObject::connect(this, SIGNAL(readyRead()), this, SLOT(processReadyRead()));
    QObject::connect(this, SIGNAL(disconnected()), &this->pingTimer, SLOT(stop()));
    QObject::connect(&this->pingTimer, SIGNAL(timeout()), this, SLOT(sendPing()));
    QObject::connect(this, SIGNAL(connected()), this, SLOT(onConnected()));
}

KegMeterConnection::~KegMeterConnection() {
}

void KegMeterConnection::processReadyRead() {
    // TODO
    //EXAMPLE:
    /*
     if (state == WaitingForGreeting) {
        if (!readProtocolHeader())
            return;
        if (currentDataType != Greeting) {
            abort();
            return;
        }
        state = ReadingGreeting;
    }

    if (state == ReadingGreeting) {
        if (!hasEnoughData())
            return;

        buffer = read(numBytesForCurrentDataType);
        if (buffer.size() != numBytesForCurrentDataType) {
            abort();
            return;
        }

        username = QString(buffer) + '@' + peerAddress().toString() + ':'
                   + QString::number(peerPort());
        currentDataType = Undefined;
        numBytesForCurrentDataType = 0;
        buffer.clear();

        if (!isValid()) {
            abort();
            return;
        }

        if (!isGreetingMessageSent) {
            sendGreetingMessage();
        }

        pingTimer.start();
        pongTime.start();
        state = ReadyForUse;
        emit readyForUse();
    }

    do {
        if (currentDataType == Undefined) {
            if (!readProtocolHeader()) {
                return;
            }
        }
        if (!hasEnoughData()) {
            return;
        }

        processData();
    }
    while (bytesAvailable() > 0);
    */
}

void KegMeterConnection::sendPing() {
    if (this->timeSinceLastResponse.elapsed() > RESPONSE_TIMEOUT_MS) {
        this->mainWindow->log("TCP Socket timeout while waiting for keg meter response.");
        this->abort();
        return;
    }

    //this->write("PING 1 p");
}

void KegMeterConnection::onConnected() {

}
