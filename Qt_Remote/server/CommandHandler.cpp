#include "CommandHandler.h"
#include <windows.h>
#include <QDateTime>


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
    case CmdType::DeleFile:
        DeleFile(body);
		break;
     case CmdType::DownLoadFile:
        DownLoadFile(body);
		break;
    case CmdType::MouseInput:
        HandleMouseEvent(body);
        break;
    case CmdType::ScreenData:
        SendScreen();
        break;
    case CmdType::LockMachine:
        LockMachine(body);
        break;
    case CmdType::UnLockMachine:
        UnlockMachine(body);
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
        info.nFileSize = file.isDir() ? 0 : file.size();
        info.nLastModified = file.lastModified().toSecsSinceEpoch();

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



void CommandHandler::HandleMouseEvent(const QByteArray& body)
{
    // 1. 安全校验数据包大小
    if (body.size() != sizeof(MouseEvent)) {
        emit logMessage("【调试】鼠标事件解析错误：包大小不匹配！");
        return;
    }

    // 2. 解包获取结构体
    MouseEvent mouse;
    memcpy(&mouse, body.constData(), sizeof(MouseEvent));

    // 3. 统一处理鼠标位置
    // 优化：在远程桌面中，主控端传来的通常是转换后的“绝对坐标”。
    // 因此，不论是点击还是单纯的 Move，都直接调用 SetCursorPos 瞬间定位。
    SetCursorPos(mouse.x, mouse.y);

    // 4. 准备 INPUT 结构体基础数据
    INPUT input = {};
    input.type = INPUT_MOUSE;
    input.mi.dx = 0; // 因为已经用 SetCursorPos 定位，这里不需要传增量
    input.mi.dy = 0;
    input.mi.mouseData = 0;
    input.mi.dwFlags = 0;

    // 5. 动作分发
    switch (mouse.eventType)
    {
    case MouseEventType::LeftPress:
        input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        SendInput(1, &input, sizeof(INPUT));
        break;
    case MouseEventType::LeftRelease:
        input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(1, &input, sizeof(INPUT));
        break;
    case MouseEventType::RightPress:
        input.mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        SendInput(1, &input, sizeof(INPUT));
        break;
    case MouseEventType::RightRelease:
        input.mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        SendInput(1, &input, sizeof(INPUT));
        break;
    case MouseEventType::MiddlePress:
        input.mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        SendInput(1, &input, sizeof(INPUT));
        break;
    case MouseEventType::MiddleRelease:
        input.mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        SendInput(1, &input, sizeof(INPUT));
        break;

        // --- 双击操作优化 ---
    case MouseEventType::LeftDoubleClick: {
        // 严谨的双击：按下->抬起->按下->抬起。使用数组一次性注入系统，防止被中断。
        INPUT inputs[4] = {};
        for (int i = 0; i < 4; ++i) inputs[i].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        inputs[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        inputs[2].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
        inputs[3].mi.dwFlags = MOUSEEVENTF_LEFTUP;
        SendInput(4, inputs, sizeof(INPUT));
        break;
    }
    case MouseEventType::RightDoubleClick: {
        INPUT inputs[4] = {};
        for (int i = 0; i < 4; ++i) inputs[i].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        inputs[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        inputs[2].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
        inputs[3].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
        SendInput(4, inputs, sizeof(INPUT));
        break;
    }
    case MouseEventType::MiddleDoubleClick: {
        INPUT inputs[4] = {};
        for (int i = 0; i < 4; ++i) inputs[i].type = INPUT_MOUSE;
        inputs[0].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        inputs[1].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        inputs[2].mi.dwFlags = MOUSEEVENTF_MIDDLEDOWN;
        inputs[3].mi.dwFlags = MOUSEEVENTF_MIDDLEUP;
        SendInput(4, inputs, sizeof(INPUT));
        break;
    }

                                          // --- 滚轮与移动 ---
    case MouseEventType::Scroll:
        input.mi.dwFlags = MOUSEEVENTF_WHEEL;
        input.mi.mouseData = mouse.scrollDelta; // 填入滚动的刻度值 (如 120 或 -120)
        SendInput(1, &input, sizeof(INPUT));
        break;

    case MouseEventType::Move:
    case MouseEventType::None:
    default:
        // Move 操作在前面的 SetCursorPos(mouse.x, mouse.y) 已经完成了真实的移动
        // 不需要再发额外的 Input 信号，直接返回即可
        break;
    }

    // 注意：对于高频触发的鼠标事件（尤其是 Move），坚决不要发回执包 (ACK)，
    // 否则会造成网络严重拥堵，所以你之前的注释掉发包逻辑是非常正确的。
}

void CommandHandler::SendScreen()
{
    // 1. 获取系统主屏幕
    QScreen* screen = QGuiApplication::primaryScreen();
    if (!screen) return;

    // 2. 截取全屏
    QPixmap pixmap = screen->grabWindow(0);

    // 3. 使用 QBuffer 代替 IStream 和 GlobalAlloc
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);

    // 【核心优化】绝对不要用 PNG！改用 JPG，并将画质设为 50-70
    // JPG 的编码速度极快，且网络包体积会缩小 5-10 倍
    //貌似PNG的画质更好
    pixmap.save(&buffer, "JPG", 50);

    //测试代码
    /*QFile file("test_screen_quality50.jpg");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(bytes);
        file.close();

        // 打印出文件大小，方便你评估网络传输压力
        emit logMessage(QString("【调试】截图已保存！大小: %1 KB").arg(bytes.size() / 1024));
    }
    else {
        emit logMessage("【调试】截图保存失败！");
    }
    */
    // 4. 打包发送 
    emit sendPacket(NetworkPacket::pack(CmdType::ScreenData, bytes));
}

void CommandHandler::LockMachine(const QByteArray&)
{
    if (m_lockWidget->isHidden()) {
        m_lockWidget->lock();
        emit logMessage("【调试】机器已锁定");
    }
    else {
        // 如果已经锁了，就回一个失败或者通知的包
        emit sendPacket(NetworkPacket::pack(CmdType::LockMachine, QByteArray()));
    }
}

void CommandHandler::UnlockMachine(const QByteArray&)
{
    m_lockWidget->unlock();

    // 发送解锁成功的回执 
    emit sendPacket(NetworkPacket::pack(CmdType::UnLockMachine, QByteArray()));
    emit logMessage("【调试】机器已解锁");
}
