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
#include "LockScreenWidget.h"
class CommandHandler :  public QObject
{
	Q_OBJECT
public:
	explicit CommandHandler(QObject* parent = nullptr) : QObject(parent) 
	{
		m_lockWidget = new LockScreenWidget();

		// 如果被控端本地有人按了 Insert 键，也要向主控端发个包通知一下（可选）
		connect(m_lockWidget, &LockScreenWidget::unlockedLocally, this, [=]() {
			emit logMessage("【调试】本地用户已通过 Insert 键解锁");
			emit sendPacket(NetworkPacket::pack(CmdType::UnLockMachine, QByteArray()));
			});
		initCommandMap();
	}

	// 定义统一的函数签名
	using ActionFunc = std::function<void(const QByteArray&)>;

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
	void HandleMouseEvent(const QByteArray& body);
	void SendScreen();
	void LockMachine(const QByteArray& /*body*/);
	void UnlockMachine(const QByteArray& /*body*/);
private:
	LockScreenWidget* m_lockWidget = nullptr;
	QHash<CmdType, ActionFunc> m_commandMap; // 指令映射表

private:
	void initCommandMap(); // 初始化路由表
};

