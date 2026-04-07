#include "ClientCommandHandler.h"
#include <QPixmap>

ClientCommandHandler::ClientCommandHandler(QObject* parent) : QObject(parent) {}

void ClientCommandHandler::onCommandReceived(CmdType type, const QByteArray& body)
{
    switch (type) {
    case CmdType::DriverInfo: {
        // 服务端发来的是 "C,D,E," 这样的字符串
        QString driveStr = QString::fromLocal8Bit(body);
        QStringList drives = driveStr.split(',', Qt::SkipEmptyParts);
        emit sigDriverInfoReceived(drives);
        break;
    }
    case CmdType::DirInfo: {
        if (body.size() == sizeof(FILEINFO)) {
            FILEINFO info;
            memcpy(&info, body.constData(), sizeof(FILEINFO));
            emit sigDirInfoReceived(info);
        }
        break;
    }
    case CmdType::ScreenData: {
        // 远端发来的是图片字节流
        QPixmap pixmap;
        if (pixmap.loadFromData(body, "JPG")) {
            emit sigScreenDataReceived(pixmap);
        }
        break;
    }
    case CmdType::RunFile: {
        emit sigLogMessage(QStringLiteral("[文件管理] 远端文件打开成功"));
        emit sigOpenFileFinished();
        break;
    }
    case CmdType::DeleFile: {
        emit sigLogMessage(QStringLiteral("[文件管理] 远端文件删除成功"));
        emit sigDeleteFileFinished();
        break;
    }
    default:
        break;
    }
}
