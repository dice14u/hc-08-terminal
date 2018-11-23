#include "hcworker.h"
#include <QBluetoothHostInfo>
#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceDiscoveryAgent>
#include <QBluetoothUuid>
#include <qdebug.h>
#include <QBluetoothSocket>
#include <QLowEnergyController>

HCWorker::HCWorker(QObject *parent) : QObject(parent)
{
    QBluetoothLocalDevice computer;
    QList<QBluetoothHostInfo> hosts = computer.allDevices();

    QBluetoothDeviceDiscoveryAgent* discovery = new QBluetoothDeviceDiscoveryAgent(this);

    connect(discovery, SIGNAL(deviceDiscovered(QBluetoothDeviceInfo)), this, SLOT(deviceDiscovered(const QBluetoothDeviceInfo)));
    connect(this, SIGNAL(hcFound(QBluetoothDeviceInfo)), this, SLOT(hcConnect(const QBluetoothDeviceInfo)));

    discovery->start();

    QString output = hosts.first().name();

}

HCWorker::~HCWorker()
{
    controller->disconnect();
}

void HCWorker::writeData(QString message)
{
    service->writeCharacteristic(writer, message.toUtf8(), QLowEnergyService::WriteWithoutResponse);
}

void HCWorker::deviceDiscovered(const QBluetoothDeviceInfo &device)
{
    auto services = device.serviceUuids();
    for(QBluetoothUuid service : services) {
        if (service == hc) {
            emit hcFound(device);
        }
    }
}

void HCWorker::hcConnect(const QBluetoothDeviceInfo &device)
{
    qDebug() << "Found new device:" << device.name() << '(' << device.address().toString() << ')';
    controller = new QLowEnergyController(device, this);

    connect(controller, &QLowEnergyController::connected, this, [this]() {
        qDebug() << "Controller connected";

        connect(controller, &QLowEnergyController::discoveryFinished, this, [this]() {
                service = controller->createServiceObject(hc, this);

                connect(service, SIGNAL(stateChanged(QLowEnergyService::ServiceState)), this, SLOT(statUpdate(QLowEnergyService::ServiceState)));

                service->discoverDetails();
        });
        controller->discoverServices();

    });

    controller->connectToDevice();

}

void HCWorker::statUpdate(QLowEnergyService::ServiceState state)
{
    if (state != QLowEnergyService::ServiceState::ServiceDiscovered) {
        return;
    }
    connect(service, SIGNAL(error(QLowEnergyService::ServiceError)), this, SLOT(handleError(QLowEnergyService::ServiceError)));

    for(auto characteristic: service->characteristics()) {
        QLowEnergyCharacteristic::PropertyTypes notifies = characteristic.properties() & (QLowEnergyCharacteristic::Notify | QLowEnergyCharacteristic::Indicate);
        if (notifies) {
            connect(service, SIGNAL(characteristicChanged(QLowEnergyCharacteristic,QByteArray)),
                        this, SLOT(newMessage(QLowEnergyCharacteristic,QByteArray)));
            QLowEnergyDescriptor notification = characteristic.descriptor(
                           QBluetoothUuid::ClientCharacteristicConfiguration);
            // turn on notifications
            if (!notification.isValid()) {
                qDebug() << "Notify bind is going to fail";
            }
            service->writeDescriptor(notification, QByteArray::fromHex("0100"));
        }
        if (characteristic.properties() & QLowEnergyCharacteristic::Read) {
            reader = characteristic;
        }
        if (characteristic.properties() & (
            QLowEnergyCharacteristic::Write
            | QLowEnergyCharacteristic::WriteNoResponse
            | QLowEnergyCharacteristic::WriteSigned)
        ) {
            writer = characteristic;
        }
    }

    qDebug() << "Service ready";
}

void HCWorker::newMessage(QLowEnergyCharacteristic, QByteArray data)
{
    emit message(data);
}

void HCWorker::handleError(QLowEnergyService::ServiceError error)
{
    qDebug() << error;
}

