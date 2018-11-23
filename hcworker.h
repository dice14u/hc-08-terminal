#ifndef HCWORKER_H
#define HCWORKER_H

#include <QObject>
#include <QBluetoothDeviceInfo>
#include <QBluetoothUuid>
#include <QLowEnergyController>

class HCWorker : public QObject
{
    Q_OBJECT
public:
    explicit HCWorker(QObject *parent = nullptr);
    virtual ~HCWorker();

    QString readData();
    void writeData(QString);

signals:
    void hcFound(const QBluetoothDeviceInfo &device);
    void message(QString);

protected slots:
    void deviceDiscovered(const QBluetoothDeviceInfo &device);
    void hcConnect(const QBluetoothDeviceInfo &device);
    void statUpdate(QLowEnergyService::ServiceState);
    void newMessage(QLowEnergyCharacteristic, QByteArray);
    void handleError(QLowEnergyService::ServiceError);

private:
    QBluetoothUuid hc = QUuid("{0000ffe0-0000-1000-8000-00805f9b34fb}");
    QLowEnergyController* controller;
    QLowEnergyService* service;
    QLowEnergyCharacteristic reader;
    QLowEnergyCharacteristic writer;
};

#endif // HCWORKER_H
