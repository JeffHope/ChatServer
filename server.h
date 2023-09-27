#pragma once
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>
#include <QMetaObject>
#include <QTimer>
#include "MessageHelper.h"
#include <QVector>
#include <QList>
#include <QSqlRecord>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QtSql/QSql>
#include <QtSql>
#include <QSqlTableModel>
#include <QTableView>

class Server : public QTcpServer {
    Q_OBJECT

public:

    Server();
    QJsonObject request_groups(const QString &login);
    static bool connectSQL();
    void sendToClient(QTcpSocket *clientSocket, const QJsonDocument& json);
    bool checkAuth(const QString &login, const QString &password, QString &dest_username);
    void accountsBase(const QString &login, const QString &password, const QString &username);
    bool checkRegistration(const QString &login, const QString &username);
    bool checkGroups(const QString& namegroup);
    void addGroup(const QString& namegroup, const QString& username);
    void updateGroup();
    void addUserInGroup(const QString& username, const QString& group);
    bool checkUserinGroup(const QString& username);
    void deleteGroup(const QString& labelgroup);
    QJsonObject returnUserInGroup(const QString& group_name);

protected:
    void prepareStatus();

private:
    QTimer *m_timer_two = nullptr;
    QTimer *m_timer = nullptr;
    QJsonDocument users_doc;
    QJsonDocument doc_group;
    QMap<QString, MessageHelper::STATUS> users_status;
    QString group_file_path = "groups.json";
    QString users_file_path = "userdata.json";
    QMap<QTcpSocket*, QString> users_sockets;
    QTcpSocket* m_socket;
    QJsonArray m_group_array;

public slots:
    void slotRead();
    void newClient();
    void broadcastStatus();

};
