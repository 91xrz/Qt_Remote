#pragma once
#include <QObject>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>

#include "NetworkData.h"

class CommandHandler :  public QObject
{
	Q_OBJECT
public:
	explicit CommandHandler(QObject* parent = nullptr) : QObject(parent) {}

public slots:
	void onHandlerCommand(CmdType type, QByteArray body);



signals:
	// 定义一个信号，专门用来把打包好的完整二进制流扔给网络层
	void sendPacket(QByteArray fullPacket);
	void logMessage(QString msg);

public:
	//业务代码
	void MakeDriverInfo();
	// 传入主控端请求的路径数据包
	void MakeDirInfo(const QByteArray& body);

	void RunFile(const QByteArray& body);
	void DeleFile(const QByteArray& body);
	void DownLoadFile(const QByteArray& body);
};

