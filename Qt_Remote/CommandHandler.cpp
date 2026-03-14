#include "CommandHandler.h"
#include <windows.h>


void  CommandHandler::onHandlerCommand(CmdType type, QByteArray body)
{
	switch (type)
	{
	case CmdType::None:
		break;
	case CmdType::DriverInfo:

		break;
	default:
		break;
	}


}

void CommandHandler::MakeDriverInfo()
{
	emit logMessage("【调试】开始获取本地磁盘信息...");

	std::string result;
	DWORD drives = GetLogicalDrives(); // 调用 Windows API 获取驱动器

	for (int i = 0; i < 26; i++) {
		if (drives & (1 << i)) {
			char driver = 'A' + i;
			result += driver;
			result += ",";
		}
	}
	emit logMessage(QString("【调试】磁盘获取成功: %1").arg(result.c_str()));

	QByteArray body(result.c_str(), result.size());
	body = NetworkPacket::pack(CmdType::DriverInfo, body);
	emit sendPacket(body);
}

void CommandHandler::MakeDirInfo()
{


}
