#include "FileManagerController.h"

#include "ClientCommandHandler.h"
#include "FileManagerWidget.h"
#include "RemoteConnection.h"

#include <QWidget>

FileManagerController::FileManagerController(RemoteConnection* connection,
    ClientCommandHandler* commandHandler,
    QWidget* parent)
    : QObject(parent)
    , m_connection(connection)
    , m_commandHandler(commandHandler)
{
    m_widget = new FileManagerWidget(parent);
    m_widget->setWindowFlags(Qt::Window);
    m_widget->setWindowTitle(QStringLiteral("远程文件管理器"));
    m_widget->resize(900, 560);

    setupConnections();
}

void FileManagerController::setupConnections()
{
    connect(m_widget, &FileManagerWidget::sigRequestDriverInfo, this, [this]() {
        m_connection->sendPacket(CmdType::DriverInfo);
    });

    connect(m_widget, &FileManagerWidget::sigRequestDirInfo, this, [this](const QString& path, bool forTree) {
        m_currentPath = path;
        m_connection->sendPacket(CmdType::DirInfo, path.toLocal8Bit());
        m_widget->setDirRequestContext(path, forTree);
    });

    connect(m_widget, &FileManagerWidget::sigRequestOpenFile, this, [this](const QString& path) {
        m_connection->sendPacket(CmdType::RunFile, path.toLocal8Bit());
    });

    connect(m_widget, &FileManagerWidget::sigRequestDeleteFile, this, [this](const QString& path) {
        m_connection->sendPacket(CmdType::DeleFile, path.toLocal8Bit());
    });

    connect(m_widget, &FileManagerWidget::sigRequestDownloadFile, this,
        [this](const QString& remotePath, const QString& localPath) {
            if (!m_commandHandler->prepareDownload(localPath)) {
                m_widget->showWarning(QStringLiteral("下载失败"), QStringLiteral("本地文件创建失败，请检查路径权限"));
                return;
            }
            m_connection->sendPacket(CmdType::DownLoadFile, remotePath.toLocal8Bit());
        });

    connect(m_commandHandler, &ClientCommandHandler::sigDriverInfoReceived,
        m_widget, &FileManagerWidget::updateDriveList);
    connect(m_commandHandler, &ClientCommandHandler::sigDirInfoReceived,
        m_widget, &FileManagerWidget::updateDirList);
    connect(m_commandHandler, &ClientCommandHandler::sigOpenFileFinished,
        m_widget, &FileManagerWidget::onOpenFileFinished);
    connect(m_commandHandler, &ClientCommandHandler::sigDeleteFileFinished, this, [this]() {
        m_widget->onDeleteFileFinished();
        if (!m_currentPath.isEmpty()) {
            m_connection->sendPacket(CmdType::DirInfo, m_currentPath.toLocal8Bit());
            m_widget->setDirRequestContext(m_currentPath, false);
        }
    });
    connect(m_commandHandler, &ClientCommandHandler::sigDownloadStarted,
        m_widget, &FileManagerWidget::showDownloadStarted);
    connect(m_commandHandler, &ClientCommandHandler::sigDownloadProgress,
        m_widget, &FileManagerWidget::showDownloadProgress);
    connect(m_commandHandler, &ClientCommandHandler::sigDownloadFinished,
        m_widget, &FileManagerWidget::showDownloadFinished);
}

void FileManagerController::show()
{
    m_widget->show();
    m_widget->raise();
    m_widget->activateWindow();

    m_connection->sendPacket(CmdType::DriverInfo);
}
