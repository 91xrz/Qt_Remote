#include "CommandHandler.h"
#include <windows.h>


void  CommandHandler::onHandlerCommand(CmdType type, QByteArray body)
{
	switch (type)
	{
	case CmdType::None:
		break;
	case CmdType::DriverInfo:
        MakeDriverInfo();
		break;
	case CmdType::DirInfo:
        MakeDirInfo(body);
		break;
    case CmdType::RunFile:
        RunFile(body);
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

//要拿到盘符或者当前的路径
void CommandHandler::MakeDirInfo(const QByteArray& body)
{
    // 1. 获取主控端请求的路径 (假设客户端把路径转成本地 8 位编码或 UTF-8 发过来)
    QString strPath = QString::fromLocal8Bit(body);
    emit logMessage(QString("【调试】开始获取目录: %1").arg(strPath));

    QDir dir(strPath);

    // 2. 如果目录不存在或没权限，发一个“结束包”回去即可
    if (!dir.exists() || !dir.isReadable()) {
        emit logMessage("【调试】目录不存在或无权限访问！");
        FILEINFO endInfo; // 结构体默认 HasNext = FALSE
        endInfo.HasNext = FALSE;
        QByteArray sendBody(reinterpret_cast<const char*>(&endInfo), sizeof(FILEINFO));
        emit sendPacket(NetworkPacket::pack(CmdType::DirInfo, sendBody));
        return;
    }

    // 3. 使用 QDir 获取文件列表，过滤掉 . 和 ..，并且让文件夹排在前面
    QFileInfoList fileList = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name);

    // 4. 循环读取并逐个发包 (这与你原有的发包逻辑完全一致)
    for (const QFileInfo& file : fileList) {
        FILEINFO info;
        info.bIsDir = file.isDir();
        info.bIsInvild = FALSE;
        info.HasNext = TRUE;

        // 安全拷贝文件名 (处理中文路径，限制最大长度为 260)
        QByteArray fileName = file.fileName().toLocal8Bit();
        qstrncpy(info.szFileName, fileName.constData(), sizeof(info.szFileName));

        // 打包发送单个文件信息
        QByteArray sendBody(reinterpret_cast<const char*>(&info), sizeof(FILEINFO));
        emit sendPacket(NetworkPacket::pack(CmdType::DirInfo, sendBody));
    }

    // 5. 遍历完毕，发送最后一个标志结束的空包
    FILEINFO endInfo;
    endInfo.HasNext = FALSE;
    QByteArray endBody(reinterpret_cast<const char*>(&endInfo), sizeof(FILEINFO));
    emit sendPacket(NetworkPacket::pack(CmdType::DirInfo, endBody));

    emit logMessage("【调试】目录信息全部发送完毕！");
}

// 接收主控端发来的包含“文件路径”的包体
void CommandHandler::RunFile(const QByteArray& body)
{
    // 1. 解析路径
    QString strPath = QString::fromLocal8Bit(body);
    emit logMessage(QString("【调试】尝试打开文件: %1").arg(strPath));

    QFileInfo fileInfo(strPath);

    // 2. 检查文件是否存在及权限 (替代 GetFileAttributes)
    if (!fileInfo.exists()) {
        emit logMessage("【调试】文件不存在！");
        // 可发送失败指令，视你的业务而定
        return;
    }
    if (!fileInfo.isReadable()) {
        emit logMessage("【调试】没有读取权限！");
        return;
    }

    // 3. 执行文件 (完美替代 ShellExecuteA，会自动调用系统默认程序打开文件或运行exe)
    bool isSuccess = QDesktopServices::openUrl(QUrl::fromLocalFile(strPath));

    if (!isSuccess) {
        emit logMessage("【调试】执行失败！");
        return;
    }

    // 4. 打包发送成功回执 (假设你的 CmdType 枚举里运行文件是 RunFile)
    // 对应你原来的 CPacket(3, NULL, 0)
    emit sendPacket(NetworkPacket::pack(CmdType::RunFile, QByteArray()));
    emit logMessage("【调试】文件执行指令已成功响应！");
}

// 接收主控端发来的包含“待删除路径”的包体
void CommandHandler::DeleFile(const QByteArray& body)
{
    // 1. 解析路径
    QString strPath = QString::fromLocal8Bit(body);
    emit logMessage(QString("【调试】尝试删除: %1").arg(strPath));

    QFileInfo fileInfo(strPath);

    // 2. 检查是否存在
    if (!fileInfo.exists()) {
        emit logMessage("【调试】删除失败：文件或目录不存在！");
        return;
    }

    bool isSuccess = false;

    // 3. 智能删除 (完美替代 DeleteFileA，且支持删文件夹)
    if (fileInfo.isDir()) {
        // 如果是文件夹，连同里面的文件一并递归删除
        QDir dir(strPath);
        isSuccess = dir.removeRecursively();
    }
    else {
        // 如果是普通文件，直接删除
        isSuccess = QFile::remove(strPath);
    }

    if (!isSuccess) {
        emit logMessage("【调试】删除失败：可能没有权限或文件正在被占用！");
        return;
    }

    // 4. 打包发送成功回执 (假设你的 CmdType 枚举里删除是 DeleteFile)
    // 对应你原来 CPacket(9, NULL, 0)
    emit sendPacket(NetworkPacket::pack(CmdType::DeleFile, QByteArray()));
    emit logMessage("【调试】删除成功，回执已发送！");
}

// 接收主控端发来的包含“待下载文件路径”的包体
void CommandHandler::DownLoadFile(const QByteArray& body)
{
    // 1. 解析请求的路径
    QString strPath = QString::fromLocal8Bit(body);
    emit logMessage(QString("【调试】准备传输文件: %1").arg(strPath));

    QFile file(strPath);

    // 2. 尝试打开文件 (只读模式)
    if (!file.open(QIODevice::ReadOnly)) {
        emit logMessage("【调试】文件打开失败，可能不存在或被占用！");
        // 文件打开失败，发送一个大小为 0 的包头回去
        qint64 zeroSize = 0;
        QByteArray errorBody(reinterpret_cast<const char*>(&zeroSize), sizeof(qint64));
        emit sendPacket(NetworkPacket::pack(CmdType::DownLoadFile,errorBody));
        return;
    }

    // 3. 获取文件大小并发送包头 (对应你原来的发送 8 字节文件大小)
    qint64 fileSize = file.size();
    QByteArray headerBody(reinterpret_cast<const char*>(&fileSize), sizeof(qint64));
    emit sendPacket(NetworkPacket::pack(CmdType::DownLoadFile,
         headerBody));
    emit logMessage(QString("【调试】文件大小: %1 字节，开始传输...").arg(fileSize));

    // 4. 分块读取并发送
    // 优化：原来是 1024 字节，这里改为 64KB (65536)，提升传输速度
    const qint64 chunkSize = 65536;

    // file.atEnd() 用来判断是否读到了文件末尾
    while (!file.atEnd()) {
        // file.read() 会自动读取指定大小的数据，如果剩余不足 64KB，就全读出来
        QByteArray chunk = file.read(chunkSize);
        emit sendPacket(NetworkPacket::pack(CmdType::DownLoadFile, chunk));
    }

    file.close();

    // 5. 传输完成，发送一个空包作为结束标志 (对应你原来的 CPacket(4, NULL, 0))
    emit sendPacket(NetworkPacket::pack(CmdType::DownLoadFile, QByteArray()));
    emit logMessage("【调试】文件传输完成！");
}
