#pragma once
#include <QObject>
#include "NetworkData.h"

class ClientCommandHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientCommandHandler(QObject* parent = nullptr);

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
};
