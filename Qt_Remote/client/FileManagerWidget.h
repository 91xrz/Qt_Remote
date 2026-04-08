#pragma once

#include <QWidget>
#include <QStringList>
#include <QProgressDialog>
#include "NetworkData.h"

class QSplitter;
class QTreeView;
class QTableView;
class QStandardItem;
class QStandardItemModel;
class QModelIndex;
class QPoint;
class RemoteConnection;
class ClientCommandHandler;

class FileManagerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FileManagerWidget(QWidget* parent = nullptr);
    void setConnection(RemoteConnection* conn);
    void setCommandHandler(ClientCommandHandler* handler);

public slots:
    void onDriverInfoReceived(const QStringList& drives);
    void onDirInfoReceived(const FILEINFO& fileInfo);
    void onOpenFileFinished();
    void onDeleteFileFinished();
    void onDownloadStarted(qint64 totalSize);
    void onDownloadProgress(qint64 receivedSize, qint64 totalSize);
    void onDownloadFinished();

private:
    void setupUi();
    void setupModels();
    void setupConnections();
    void requestTestDrives();
    void addDummyNode(QStandardItem* parentItem);
    bool hasDummyChild(QStandardItem* parentItem) const;
    void populateChildrenForItem(QStandardItem* parentItem);
    void showFilesForIndex(const QModelIndex& index);
    void showFileContextMenu(const QPoint& pos);

private:
    QSplitter* m_splitter;
    QTreeView* m_treeView;
    QTableView* m_tableView;
    QStandardItemModel* m_treeModel;
    QStandardItemModel* m_tableModel;
    RemoteConnection* m_connection = nullptr;
    ClientCommandHandler* m_cmdHandler = nullptr;
    QProgressDialog* m_progressDlg = nullptr;
    bool m_isRequestingTree = false;
    QStandardItem* m_expandingItem = nullptr;
    QString m_currentRequestPath;
};
