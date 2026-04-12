#pragma once

#include <QObject>
#include <QString>

class FileManagerWidget;
class RemoteConnection;
class ClientCommandHandler;
class QWidget;

class FileManagerController : public QObject
{
    Q_OBJECT
public:
    explicit FileManagerController(RemoteConnection* connection,
        ClientCommandHandler* commandHandler,
        QWidget* parent = nullptr);

    void show();

private:
    void setupConnections();

private:
    RemoteConnection* m_connection = nullptr;
    ClientCommandHandler* m_commandHandler = nullptr;
    FileManagerWidget* m_widget = nullptr;
    QString m_currentPath;
};
