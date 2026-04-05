#include "FileManagerWidget.h"

#include <QAction>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QHeaderView>
#include <QStyle>
#include <QItemSelectionModel>
#include <QMenu>
#include <QModelIndex>
#include <QSplitter>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTableView>
#include <QTreeView>
#include <QVBoxLayout>

namespace {
constexpr auto kDummyText = "Loading...";
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
    loadTestDrives();
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
        qDebug() << "正在向远端请求数据..." << item->text();
        populateChildrenForItem(item);
    });

    connect(m_treeView->selectionModel(), &QItemSelectionModel::currentChanged, this,
        [this](const QModelIndex& current, const QModelIndex&) {
            showFilesForIndex(current);
        });

    connect(m_tableView, &QWidget::customContextMenuRequested, this, &FileManagerWidget::showFileContextMenu);
}

void FileManagerWidget::loadTestDrives()
{
    const QIcon driveIcon = QApplication::style()->standardIcon(QStyle::SP_DriveHDIcon);

    const QStringList drives = { QStringLiteral("C:"), QStringLiteral("D:") };
    for (const QString& driveName : drives) {
        auto* driveItem = new QStandardItem(driveIcon, driveName);
        addDummyNode(driveItem);
        m_treeModel->appendRow(driveItem);
    }

    if (m_treeModel->rowCount() > 0) {
        const QModelIndex firstIndex = m_treeModel->index(0, 0);
        m_treeView->setCurrentIndex(firstIndex);
        showFilesForIndex(firstIndex);
    }
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

    const QIcon dirIcon = QApplication::style()->standardIcon(QStyle::SP_DirIcon);
    const QIcon fileIcon = QApplication::style()->standardIcon(QStyle::SP_FileIcon);

    for (int i = 1; i <= 2; ++i) {
        auto* folderItem = new QStandardItem(dirIcon, QStringLiteral("Folder_%1").arg(i));
        addDummyNode(folderItem);
        parentItem->appendRow(folderItem);
    }

    for (int i = 1; i <= 2; ++i) {
        auto* fileItem = new QStandardItem(fileIcon, QStringLiteral("Readme_%1.txt").arg(i));
        parentItem->appendRow(fileItem);
    }
}

void FileManagerWidget::showFilesForIndex(const QModelIndex& index)
{
    m_tableModel->removeRows(0, m_tableModel->rowCount());
    if (!index.isValid()) {
        return;
    }

    const QString parentName = m_treeModel->itemFromIndex(index)->text();
    const QString now = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    for (int i = 1; i <= 5; ++i) {
        QList<QStandardItem*> row;
        const bool isDir = (i % 2 == 0);
        const QIcon icon = QApplication::style()->standardIcon(isDir ? QStyle::SP_DirIcon : QStyle::SP_FileIcon);
        row << new QStandardItem(icon, QStringLiteral("%1_Item_%2").arg(parentName).arg(i));
        row << new QStandardItem(isDir ? QStringLiteral("--") : QStringLiteral("%1 KB").arg(i * 128));
        row << new QStandardItem(isDir ? QStringLiteral("文件夹") : QStringLiteral("文本文件"));
        row << new QStandardItem(now);
        m_tableModel->appendRow(row);
    }
}

void FileManagerWidget::showFileContextMenu(const QPoint& pos)
{
    const QModelIndex index = m_tableView->indexAt(pos);
    if (!index.isValid()) {
        return;
    }

    const QString fileName = m_tableModel->item(index.row(), 0)->text();

    QMenu menu(this);
    QAction* openAction = menu.addAction(QStringLiteral("打开"));
    QAction* downloadAction = menu.addAction(QStringLiteral("下载"));
    QAction* deleteAction = menu.addAction(QStringLiteral("删除"));
    QAction* runAction = menu.addAction(QStringLiteral("运行"));

    QAction* selectedAction = menu.exec(m_tableView->viewport()->mapToGlobal(pos));
    if (!selectedAction) {
        return;
    }

    if (selectedAction == openAction) {
        qDebug() << "[文件管理] 打开:" << fileName;
    }
    else if (selectedAction == downloadAction) {
        qDebug() << "[文件管理] 下载:" << fileName;
    }
    else if (selectedAction == deleteAction) {
        qDebug() << "[文件管理] 删除:" << fileName;
    }
    else if (selectedAction == runAction) {
        qDebug() << "[文件管理] 运行:" << fileName;
    }
}
