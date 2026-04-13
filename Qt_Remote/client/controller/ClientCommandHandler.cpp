#include "ClientCommandHandler.h"
#include <QPixmap>

ClientCommandHandler::ClientCommandHandler(QObject* parent) : QObject(parent) {

    initCommandMap();
}

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

void ClientCommandHandler::cancelLocalDownload()
{
    const QString localPath = m_downloadFile.fileName();
    if (m_downloadFile.isOpen()) {
        m_downloadFile.close();
    }
    if (!localPath.isEmpty()) {
        QFile::remove(localPath);
    }

    m_expectedSize = 0;
    m_receivedSize = 0;
    m_isDownloading = false;
    emit sigLogMessage(QStringLiteral("[文件下载] 本地下载已取消，已清理未完成文件"));
}

void ClientCommandHandler::initCommandMap()
{
    m_commandMap[CmdType::DriverInfo] = [this](const QByteArray& body) {
        QString driveStr = QString::fromLocal8Bit(body);
        emit sigDriverInfoReceived(driveStr.split(',', Qt::SkipEmptyParts));
        };

    m_commandMap[CmdType::DirInfo] = [this](const QByteArray& body) {
        if (body.size() == sizeof(FILEINFO)) {
            FILEINFO info;
            memcpy(&info, body.constData(), sizeof(FILEINFO));
            emit sigDirInfoReceived(info);
        }
        };

    m_commandMap[CmdType::ScreenData] = [this](const QByteArray& body) {
        QPixmap pixmap;
        if (pixmap.loadFromData(body, "JPG")) {
            emit sigScreenDataReceived(pixmap);
        }
        };

    m_commandMap[CmdType::RunFile] = [this](const QByteArray&) {
        emit sigLogMessage(QStringLiteral("[文件管理] 远端文件打开成功"));
        emit sigOpenFileFinished();
        };

    m_commandMap[CmdType::DeleFile] = [this](const QByteArray&) {
        emit sigLogMessage(QStringLiteral("[文件管理] 远端文件删除成功"));
        emit sigDeleteFileFinished();
        };

    m_commandMap[CmdType::LockMachine] = [this](const QByteArray& body) {
        if (!body.isEmpty() && static_cast<LockResult>(body.at(0)) == LockResult::LockSuccess) {
            emit sigLogMessage("[远程控制] 锁机指令执行成功");
        }
        else {
            emit sigLogMessage("[远程控制] 锁机失败或已锁定");
        }
        };

    m_commandMap[CmdType::UnLockMachine] = [this](const QByteArray&) {
        emit sigLogMessage(QStringLiteral("[远程控制] 解锁指令执行成功"));
        };

    // 针对逻辑较长的下载，建议单独抽离为一个类方法 handleDownloadFile
    m_commandMap[CmdType::DownLoadFile] = [this](const QByteArray& body) {
        handleDownloadFile(body);
        };
    m_commandMap[CmdType::DownloadNextChunk] = [this](const QByteArray& body) {
        handleDownloadFile(body);
        };
}

void ClientCommandHandler::handleDownloadFile(const QByteArray& body)
{
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
            return;
        }

        m_expectedSize = totalSize;
        m_receivedSize = 0;
        m_isDownloading = true;
        emit sigDownloadStarted(m_expectedSize);
        emit sigLogMessage(QStringLiteral("[文件下载] 开始下载，总大小: %1 字节").arg(m_expectedSize));
        emit sigRequestDownloadNextChunk();
        return;
    }

    if (m_isDownloading) {
        if (body.isEmpty()) {
            if (m_downloadFile.isOpen()) {
                m_downloadFile.close();
            }
            m_isDownloading = false;
            emit sigDownloadFinished();
            emit sigLogMessage(QStringLiteral("[文件下载] 下载完成，保存路径: %1").arg(m_downloadFile.fileName()));
            return;
        }

        if (!m_downloadFile.isOpen()) {
            emit sigLogMessage(QStringLiteral("[文件下载] 本地文件未打开，写入失败"));
            m_isDownloading = false;
            return;
        }

        const qint64 written = m_downloadFile.write(body);
        if (written < 0) {
            emit sigLogMessage(QStringLiteral("[文件下载] 写入本地文件失败"));
            cancelLocalDownload();
            return;
        }

        m_receivedSize += written;
        emit sigDownloadProgress(m_receivedSize, m_expectedSize);
        emit sigRequestDownloadNextChunk();
    }
}

void ClientCommandHandler::onCommandReceived(CmdType type, const QByteArray& body)
{
    auto it = m_commandMap.find(type);
    if (it != m_commandMap.end()) {
        it.value()(body);
    }
}
