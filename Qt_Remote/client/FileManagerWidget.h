#pragma once

#include <QWidget>

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

private:
    void setupUi();
    void setupModels();
    void setupConnections();
    void loadTestDrives();
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
};
