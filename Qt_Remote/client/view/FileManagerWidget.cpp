#include "FileManagerWidget.h"
#include "NetworkData.h"

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMenu>
#include <QMessageBox>
#include <QModelIndex>
#include <QSplitter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QStyle>
#include <QTableView>
#include <QTreeView>
#include <QVBoxLayout>

namespace {
constexpr auto kDummyText = "Loading...";
}

static QString formatSize(qint64 bytes)
{
    double size = bytes;
    QStringList units = { "B", "KB", "MB", "GB", "TB" };

    int i = 0;
    while (size >= 1024 && i < units.size() - 1) {
        size /= 1024;
        ++i;
    }

    return QString::number(size, 'f', 2) + " " + units[i];
}

FileManagerWidget::FileManagerWidget(QWidget* parent)
    : QWidget(parent)
    , m_splitter(nullptr)
    , m_treeView(nullptr)
    , m_tableView(nullptr)
    , m_treeModel(new QStandardItemModel(this))
    , m_tableModel(new QStandardItemModel(this))
{
    setupUi();
    setupModels();
    setupConnections();
}

void FileManagerWidget::setDirRequestContext(const QString& requestPath, bool requestTree)
{
    m_currentRequestPath = requestPath;
    m_isRequestingTree = requestTree;
    if (!requestTree) {
        m_expandingItem = nullptr;
    }
}

void FileManagerWidget::showWarning(const QString& title, const QString& message)
{
    QMessageBox::warning(this, title, message);
}

void FileManagerWidget::setupUi()
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_treeView = new QTreeView(m_splitter);
    m_tableView = new QTableView(m_splitter);

    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 2);
    layout->addWidget(m_splitter);
}

void FileManagerWidget::setupModels()
{
    m_treeModel->setHorizontalHeaderLabels({ QStringLiteral("远端目录") });
    m_treeView->setModel(m_treeModel);

    m_tableModel->setHorizontalHeaderLabels(
        { QStringLiteral("文件名"), QStringLiteral("大小"), QStringLiteral("类型"), QStringLiteral("修改时间") });
    m_tableView->setModel(m_tableModel);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setAlternatingRowColors(true);
    m_tableView->horizontalHeader()->setStretchLastSection(true);
    m_tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableView->setContextMenuPolicy(Qt::CustomContextMenu);
}

void FileManagerWidget::setupConnections()
{
    connect(m_treeView, &QTreeView::expanded, this, [this](const QModelIndex& index) {
        if (!index.isValid()) {
            return;
        }

        auto* item = m_treeModel->itemFromIndex(index);
        if (!item || !hasDummyChild(item)) {
            return;
        }

        item->removeRows(0, item->rowCount());
        populateChildrenForItem(item);
    });

    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
        [this](const QModelIndex& current, const QModelIndex&) {
            showFilesForIndex(current);
        });

    connect(m_tableView, &QWidget::customContextMenuRequested, this, &FileManagerWidget::showFileContextMenu);
}

void FileManagerWidget::addDummyNode(QStandardItem* parentItem)
{
    if (!parentItem) {
        return;
    }
    parentItem->appendRow(new QStandardItem(QString::fromUtf8(kDummyText)));
}

bool FileManagerWidget::hasDummyChild(QStandardItem* parentItem) const
{
    if (!parentItem || parentItem->rowCount() != 1) {
        return false;
    }

    QStandardItem* firstChild = parentItem->child(0, 0);
    return firstChild && firstChild->text() == QString::fromUtf8(kDummyText);
}

void FileManagerWidget::populateChildrenForItem(QStandardItem* parentItem)
{
    if (!parentItem) {
        return;
    }

    m_expandingItem = parentItem;
    const QString path = parentItem->data(Qt::UserRole + 1).toString();
    emit sigRequestDirInfo(path, true);
}

void FileManagerWidget::showFilesForIndex(const QModelIndex& index)
{
    m_tableModel->removeRows(0, m_tableModel->rowCount());
    if (!index.isValid()) {
        return;
    }

    QStandardItem* item = m_treeModel->itemFromIndex(index);
    if (!item) {
        return;
    }

    const QString path = item->data(Qt::UserRole + 1).toString();
    emit sigRequestDirInfo(path, false);
}

void FileManagerWidget::updateDriveList(const QStringList& drives)
{
    m_treeModel->removeRows(0, m_treeModel->rowCount());
    const QIcon driveIcon = QApplication::style()->standardIcon(QStyle::SP_DriveHDIcon);

    for (const QString& driveName : drives) {
        QString drivePath = driveName.trimmed();
        if (drivePath.isEmpty()) {
            continue;
        }
        if (!drivePath.endsWith(':')) {
            drivePath.append(':');
        }
        drivePath.append('\\');

        auto* driveItem = new QStandardItem(driveIcon, drivePath);
        driveItem->setData(drivePath, Qt::UserRole + 1);
        addDummyNode(driveItem);
        m_treeModel->appendRow(driveItem);
    }
}

void FileManagerWidget::updateDirList(const FILEINFO& fileInfo)
{
    if (!fileInfo.HasNext) {
        m_expandingItem = nullptr;
        return;
    }

    const QString basePath = m_currentRequestPath;
    const QString name = QString::fromLocal8Bit(fileInfo.szFileName);
    QString fullPath = basePath;
    if (!fullPath.endsWith('\\') && !fullPath.endsWith('/')) {
        fullPath.append('\\');
    }
    fullPath.append(name);

    if (m_isRequestingTree) {
        if (!m_expandingItem || !fileInfo.bIsDir) {
            return;
        }
        const QIcon dirIcon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
        auto* folderItem = new QStandardItem(dirIcon, name);
        folderItem->setData(fullPath, Qt::UserRole + 1);
        addDummyNode(folderItem);
        m_expandingItem->appendRow(folderItem);
        return;
    }

    QList<QStandardItem*> row;
    const bool isDir = fileInfo.bIsDir;
    const QIcon icon = QApplication::style()->standardIcon(isDir ? QStyle::SP_DirIcon : QStyle::SP_FileIcon);

    auto* nameItem = new QStandardItem(icon, name);
    nameItem->setData(fullPath, Qt::UserRole + 1);
    row << nameItem;
    row << new QStandardItem(isDir ? QStringLiteral("--") : formatSize(fileInfo.nFileSize));
    row << new QStandardItem(isDir ? QStringLiteral("文件夹") : QStringLiteral("文件"));
    const QString lastModified = fileInfo.nLastModified > 0
        ? QDateTime::fromSecsSinceEpoch(fileInfo.nLastModified).toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"))
        : QStringLiteral("--");
    row << new QStandardItem(lastModified);
    m_tableModel->appendRow(row);
}

void FileManagerWidget::showFileContextMenu(const QPoint& pos)
{
    const QModelIndex index = m_tableView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    QStandardItem* nameItem = m_tableModel->item(index.row(), 0);
    if (!nameItem) {
        return;
    }

    const QString fileName = nameItem->text();
    const QString fullPath = nameItem->data(Qt::UserRole + 1).toString();

    QMenu menu(this);
    QAction* openAction = menu.addAction(QStringLiteral("打开"));
    QAction* downloadAction = menu.addAction(QStringLiteral("下载"));
    QAction* deleteAction = menu.addAction(QStringLiteral("删除"));

    QAction* selectedAction = menu.exec(m_tableView->viewport()->mapToGlobal(pos));
    if (!selectedAction) {
        return;
    }

    if (selectedAction == openAction) {
        emit sigRequestOpenFile(fullPath);
    }
    else if (selectedAction == downloadAction) {
        const QString localPath = QFileDialog::getSaveFileName(this, QStringLiteral("保存文件"), fileName);
        if (localPath.isEmpty()) {
            return;
        }
        emit sigRequestDownloadFile(fullPath, localPath);
    }
    else if (selectedAction == deleteAction) {
        emit sigRequestDeleteFile(fullPath);
    }
}

void FileManagerWidget::onOpenFileFinished()
{
    qDebug() << "[文件管理] 远端文件已打开";
}

void FileManagerWidget::onDeleteFileFinished()
{
    qDebug() << "[文件管理] 远端文件已删除";
    m_tableModel->removeRows(0, m_tableModel->rowCount());
}

void FileManagerWidget::showDownloadStarted(qint64 totalSize)
{
    if (m_progressDlg) {
        m_progressDlg->deleteLater();
        m_progressDlg = nullptr;
    }

    m_progressDlg = new QProgressDialog(QStringLiteral("准备下载..."), QString(), 0, 100, this);
    m_progressDlg->setWindowModality(Qt::WindowModal);
    m_progressDlg->setCancelButtonText(QStringLiteral("取消下载"));
    m_progressDlg->setMinimumDuration(0);
    m_progressDlg->setValue(0);
    m_progressDlg->setLabelText(QStringLiteral("文件总大小: %1 MB").arg(totalSize / 1024.0 / 1024.0, 0, 'f', 2));
    connect(m_progressDlg, &QProgressDialog::canceled, this, [this]() {
        emit sigCancelDownload();
    });
    m_progressDlg->show();
}

void FileManagerWidget::showDownloadProgress(qint64 receivedSize, qint64 totalSize)
{
    if (!m_progressDlg || totalSize <= 0) {
        return;
    }

    const int progress = static_cast<int>((receivedSize * 100) / totalSize);
    m_progressDlg->setValue(qBound(0, progress, 100));
    m_progressDlg->setLabelText(QStringLiteral("已下载: %1 MB / %2 MB")
            .arg(receivedSize / 1024.0 / 1024.0, 0, 'f', 2)
            .arg(totalSize / 1024.0 / 1024.0, 0, 'f', 2));
}

void FileManagerWidget::showDownloadFinished()
{
    if (m_progressDlg) {
        m_progressDlg->setValue(100);
        m_progressDlg->hide();
        m_progressDlg->deleteLater();
        m_progressDlg = nullptr;
    }

    QMessageBox::information(this, QStringLiteral("下载完成"), QStringLiteral("文件下载完成"));
}
