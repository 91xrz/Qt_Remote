#pragma once

#include <QObject>
#include <QByteArray>
#include <QQueue>

#include "NetworkData.h"

class PacketStreamWriter : public QObject
{
    Q_OBJECT

public:
    explicit PacketStreamWriter(QObject* parent = nullptr);

public slots:
    void enqueue(CmdType type, const QByteArray& body);

signals:
    void writeToSocket(const QByteArray& rawData);

private slots:
    void processQueue();

private:
    QQueue<QByteArray> m_sendQueue;
};
