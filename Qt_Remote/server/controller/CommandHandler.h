#pragma once
#include <QObject>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <windows.h>
#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QBuffer>
#include "NetworkData.h"

class CommandHandler : public QObject
{
    Q_OBJECT
public:
    explicit CommandHandler(QObject* parent = nullptr);

    using ActionFunc = std::function<void(const QByteArray&)>;

public slots:
    void onHandlerCommand(CmdType type, QByteArray body);

signals:
    void sendData(CmdType type, QByteArray body);
    void logMessage(QString msg);
    void sigLockMachineRequested();
    void sigUnlockMachineRequested();

public:
    void MakeDriverInfo();
    void MakeDirInfo(const QByteArray& body);
    void RunFile(const QByteArray& body);
    void DeleFile(const QByteArray& body);
    void DownLoadFile(const QByteArray& body);
    void HandleMouseEvent(const QByteArray& body);
    void HandleKeyboardEvent(const QByteArray& body);
    void SendScreen();
    void LockMachine(const QByteArray& body);
    void UnlockMachine(const QByteArray& body);

private:
    QHash<CmdType, ActionFunc> m_commandMap;

    void initCommandMap();
};
