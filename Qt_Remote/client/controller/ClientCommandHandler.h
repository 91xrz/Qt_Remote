#pragma once
#include <QObject>
#include <QFile>
#include "NetworkData.h"

class ClientCommandHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientCommandHandler(QObject* parent = nullptr);
    bool prepareDownload(const QString& localPath);

private:
    using ActionFunc = std::function<void(const QByteArray&)>;
    QHash<CmdType, ActionFunc> m_commandMap;
    void initCommandMap();

    // 将 switch 里的庞大逻辑抽离成私有函数
    void handleDownloadFile(const QByteArray& body);
public slots:
    // 接收来自 RemoteConnection 的原始数据包
    void onCommandReceived(CmdType type, const QByteArray& body);

signals:
    // 定义具体的业务信号，给 UI 层去绑定
    void sigLogMessage(const QString& msg);

    // 磁盘信息信号 (例如："C", "D", "E")
    void sigDriverInfoReceived(const QStringList& drives);

    // 目录/文件信息信号
    void sigDirInfoReceived(const FILEINFO& fileInfo);

    // 屏幕截图数据接收信号
    void sigScreenDataReceived(const QPixmap& pixmap);

    // 打开文件回执
    void sigOpenFileFinished();

    // 删除文件回执
    void sigDeleteFileFinished();

    // 下载流程信号
    void sigDownloadStarted(qint64 totalSize);
    void sigDownloadProgress(qint64 receivedSize, qint64 totalSize);
    void sigDownloadFinished();

private:
    QFile m_downloadFile;
    qint64 m_expectedSize = 0;
    qint64 m_receivedSize = 0;
    bool m_isDownloading = false;

   
};
