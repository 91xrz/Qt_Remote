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

class FileManagerWidget : public QWidget
{
    Q_OBJECT

public:
    explicit FileManagerWidget(QWidget* parent = nullptr);

    void setDirRequestContext(const QString& requestPath, bool requestTree);
    void showWarning(const QString& title, const QString& message);

signals:
    void sigRequestDriverInfo();
    void sigRequestDirInfo(const QString& path, bool forTree);
    void sigRequestOpenFile(const QString& path);
    void sigRequestDeleteFile(const QString& path);
    void sigRequestDownloadFile(const QString& remotePath, const QString& localPath);
    void sigCancelDownload();

public slots:
    void updateDriveList(const QStringList& drives);
    void updateDirList(const FILEINFO& fileInfo);
    void onOpenFileFinished();
    void onDeleteFileFinished();
    void showDownloadStarted(qint64 totalSize);
    void showDownloadProgress(qint64 receivedSize, qint64 totalSize);
    void showDownloadFinished();

private:
    void setupUi();
    void setupModels();
    void setupConnections();
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
    QProgressDialog* m_progressDlg = nullptr;
    bool m_isRequestingTree = false;
    QStandardItem* m_expandingItem = nullptr;
    QString m_currentRequestPath;
};
