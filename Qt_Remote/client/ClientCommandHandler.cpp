#include "ClientCommandHandler.h"
#include <QPixmap>

ClientCommandHandler::ClientCommandHandler(QObject* parent) : QObject(parent) {}

bool ClientCommandHandler::prepareDownload(const QString& localPath)
{
    if (m_downloadFile.isOpen()) {
        m_downloadFile.close();
    }

    m_downloadFile.setFileName(localPath);
    if (!m_downloadFile.open(QIODevice::WriteOnly)) {
        emit sigLogMessage(QStringLiteral("[文件下载] 本地文件创建失败: %1").arg(localPath));
        return false;
    }

    m_expectedSize = 0;
    m_receivedSize = 0;
    m_isDownloading = false;
    return true;
}

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
    case CmdType::DownLoadFile: {
        if (!m_isDownloading && body.size() == sizeof(qint64)) {
            qint64 totalSize = 0;
            memcpy(&totalSize, body.constData(), sizeof(qint64));

            if (totalSize <= 0) {
                const QString localPath = m_downloadFile.fileName();
                if (m_downloadFile.isOpen()) {
                    m_downloadFile.close();
                }
                if (!localPath.isEmpty()) {
                    m_downloadFile.remove();
                }
                emit sigLogMessage(QStringLiteral("[文件下载] 远端文件不存在或打开失败"));
                break;
            }

            m_expectedSize = totalSize;
            m_receivedSize = 0;
            m_isDownloading = true;
            emit sigDownloadStarted(m_expectedSize);
            emit sigLogMessage(QStringLiteral("[文件下载] 开始下载，总大小: %1 字节").arg(m_expectedSize));
            break;
        }

        if (m_isDownloading) {
            if (body.isEmpty()) {
                if (m_downloadFile.isOpen()) {
                    m_downloadFile.close();
                }
                m_isDownloading = false;
                emit sigDownloadFinished();
                emit sigLogMessage(QStringLiteral("[文件下载] 下载完成，保存路径: %1").arg(m_downloadFile.fileName()));
                break;
            }

            if (!m_downloadFile.isOpen()) {
                emit sigLogMessage(QStringLiteral("[文件下载] 本地文件未打开，写入失败"));
                m_isDownloading = false;
                break;
            }

            const qint64 written = m_downloadFile.write(body);
            if (written < 0) {
                emit sigLogMessage(QStringLiteral("[文件下载] 写入本地文件失败"));
                m_isDownloading = false;
                m_downloadFile.close();
                break;
            }

            m_receivedSize += written;
            emit sigDownloadProgress(m_receivedSize, m_expectedSize);
        }
        break;
    }
    case CmdType::LockMachine: {
        auto result = static_cast<LockResult>(body.at(0));

        if (result == LockResult::LockSuccess) {
            emit sigLogMessage("[远程控制] 锁机指令执行成功");
        }
        else {
            emit sigLogMessage("[远程控制] 锁机失败或已锁定");
        }
        break;
    }
    case CmdType::UnLockMachine: {
        emit sigLogMessage(QStringLiteral("[远程控制] 解锁指令执行成功"));
        break;
    }
    default:
        break;
    }
}
