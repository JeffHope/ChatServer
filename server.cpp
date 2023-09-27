#include "server.h"
#include <QDataStream>
#include <QThread>
#include <QTime>
#include <QMetaEnum>
#include <QMetaObject>
#include <QApplication>
#include <QMessageBox>

Server::Server() {
    m_timer = new QTimer(this);
    m_timer->setInterval(1000);
    m_timer->start();

    ///////SQL//////
    connectSQL();
    request_groups("log");
    ////////////////

    ///////////USERS//////////
    QFile file(users_file_path);
    if (!file.exists()) {
        qCritical() << "File of users is not exist";
        QApplication::exit(-1);
    }

    if (!file.open(QIODevice::ReadOnly)) {
        qCritical() << "Cannot open file";
        QApplication::exit(-1);
    }

    users_doc = QJsonDocument::fromJson(file.readAll());

    file.close();

    /////////GROUPS/////////

    QFile file_groups(group_file_path);
    if(!file_groups.exists()){
        QApplication::exit(-1);
    }
    if(!file_groups.open(QIODevice::ReadOnly)){
        QApplication::exit(-1);
    }
    doc_group = QJsonDocument::fromJson(file_groups.readAll());
    qDebug () << "data reading" << doc_group;
    file_groups.close();
    updateGroup();


    this->setMaxPendingConnections(10);

    connect(this, &Server::newConnection, this, &Server::newClient);
    connect(m_timer, &QTimer::timeout, this, &Server::broadcastStatus);
    prepareStatus();

}

QJsonObject Server::request_groups(const QString &login) {
    QSqlQuery query_user_in_groups, query_groups, query_user_groups;
    QString user_name, group_name, user_in_groups;
    int login_id, group_id, users_id_key, groups_id_key = 0;
    query_user_in_groups.prepare(
        "SELECT DISTINCT group_id, group_name FROM groups "
        "INNER JOIN user_groups ON groups.id = user_groups.group_id "
        "INNER JOIN users ON user_groups.login_id = users.id "
            "WHERE users.login = ? "
    );
    query_user_in_groups.addBindValue(login);
    query_user_in_groups.exec();

    QJsonDocument json_doc;
    QJsonObject json;
    QJsonArray groups_json;
    while(query_user_in_groups.next()) {
        
        group_id = query_user_in_groups.value(0).toInt();
        group_name = query_user_in_groups.value(1).toString();
        query_groups.prepare(
            "SELECT group_id, login from users "
            "INNER JOIN user_groups ON users.id = user_groups.login_id "
            "INNER JOIN groups ON user_groups.group_id = groups.id "
        );
        query_groups.exec();

        QJsonArray users_in_group_json;
        QJsonObject group;
        
        while(query_groups.next()) {
            groups_id_key = query_groups.value(0).toInt();
            if(groups_id_key == group_id) {
                users_in_group_json.append(query_groups.value(1).toString());
            }
        }
        group["groupname"] = group_name;
        group["users"] = users_in_group_json;

        groups_json.append(group);
    }
    json["groups"] = groups_json;
    json_doc.setObject(json);
    QString jsonString = json_doc.toJson(QJsonDocument::Indented);
    qDebug() << jsonString.toStdString().c_str();
    return json;
}

void Server::updateGroup() {
    QFile file(group_file_path);
    file.open(QIODevice::ReadOnly);
    QJsonObject json = doc_group.object();
    QJsonArray arr = json["groups"].toArray();
    for(int i = 0; i < arr.size(); ++ i){
        QJsonObject tmp = arr[i].toObject();
        QString str = tmp.value("namegroup").toString();
        QJsonArray arrUser = tmp.value("namegroup").toArray();
        for(int i = 0; i < arrUser.size(); ++i){
            QJsonObject tmp_us = arrUser[i].toObject();
            QString usStr = tmp_us.value("username").toString();
            qDebug() << "user in groups: " << usStr;
        }
    }
    file.close();
}

void Server::prepareStatus() {
    QJsonObject users_json = users_doc.object();
    QJsonArray src_accounts = users_json["accounts"].toArray();
    for (const auto &account : src_accounts) {
        QJsonObject src_obj = account.toObject();
        QString username = src_obj["username"].toString();
        users_status[username] = MessageHelper::STATUS::Offline;
    }
}

void Server::broadcastStatus() {
    foreach (QTcpSocket* socket, this->findChildren<QTcpSocket*>()) {
        QJsonArray accounts;
        for (const auto &username : users_status.keys()) {
            QJsonObject user_obj;
            user_obj["username"] = username;
            user_obj["status"] = MessageHelper::enumStatusToString(users_status[username]);
            accounts.append(user_obj);
        }
        QJsonObject dest_obj, group_obj;
        dest_obj["accounts"] = accounts;
        group_obj = doc_group.object();
        QJsonArray arr;
        arr.append(dest_obj);
        arr.append(group_obj);
        QJsonObject json;
        json["objects"] = arr;
        socket->write(MessageHelper::make(MessageHelper::TYPE::Broadcast, json));
    }

    prepareStatus();
}

bool Server::checkAuth(const QString &login, const QString &password, QString &username) {

    QJsonObject users_json = users_doc.object();
    QJsonArray arr = users_json["accounts"].toArray();
    for (const auto &account : arr) {
        QJsonObject obj = account.toObject();
        if (obj["login"] == login) {
            if (obj["password"] == password) {
                username = obj["username"].toString();
                return true;
            }
            continue;
        }
    }

    return false;
}


bool Server::connectSQL(){
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("127.0.0.1");
    db.setDatabaseName("chat");
    db.setUserName("sunny");
    db.setPassword("123456");
    bool ok = db.open();
    if(!ok){
        qDebug() << "Cannot open databse";
        qDebug() << "DB last error: " << db.lastError();
        qDebug() << "DB connection names: "  << db.connectionNames();
        qDebug() << "DB connection options: "  << db.connectOptions();
        qDebug() << "DB port: "  << db.port();
        return false;
    }else{
        qDebug() << "DB is open";
    }

    QStringList strlsit = db.tables();
    foreach(QString str, strlsit){
        qDebug() << "PSQL Tables: " << str;
    }

    return true;

}

void Server::newClient() {
    QTcpSocket *mSocket = this->nextPendingConnection();
    connect(mSocket, &QTcpSocket::readyRead, this, &Server::slotRead);
    qDebug() << "Client connected ";
}

void Server::slotRead() {
    QTcpSocket *sender_socket = (QTcpSocket*) sender();
    QByteArray data = sender_socket->readAll();
    QJsonDocument recv_doc = QJsonDocument::fromBinaryData(data);
    QJsonObject recv_json = recv_doc.object();

    if(recv_json.contains("type")) {
        if (recv_json["type"] == MessageHelper::enumTypeToString(MessageHelper::TYPE::Message)) {
            foreach (QTcpSocket *socket, this->findChildren<QTcpSocket *>()) {
                sendToClient(socket, recv_doc);
            }
        }

        if (recv_json["type"] == MessageHelper::enumTypeToString(MessageHelper::TYPE::Status)) {
            QString username = recv_json["username"].toString();
            QString status = recv_json["status"].toString();

            if(users_sockets[sender_socket] == username) {
                users_status[username] = MessageHelper::statusStringToEnum(status);
            }
        }

        if (recv_json["type"] == MessageHelper::enumTypeToString(MessageHelper::TYPE::Auth)) {
            QString auth_username = "";
            qDebug () << auth_username;
            if (!checkAuth(recv_json["login"].toString(), recv_json["password"].toString(), auth_username)) {
                sender_socket->write(MessageHelper::makeError());
                return;
            }
            if(users_status[auth_username] != MessageHelper::STATUS::Offline) {
                sender_socket->write(MessageHelper::makeDoubleSignError());
                return;
            }
            sender_socket->write(MessageHelper::makeAccess(auth_username));

            users_sockets[sender_socket] = auth_username;



        }
        if(recv_json["type"] == MessageHelper::enumTypeToString(MessageHelper::TYPE::Registration)) {
            if(!checkRegistration(recv_json["login"].toString(), recv_json["username"].toString())) {
                sender_socket->write(MessageHelper::makeErrorRegistration());
            } else {
                accountsBase(recv_json["login"].toString(), recv_json["password"].toString(), recv_json["username"].toString());
                sender_socket->write(MessageHelper::makeAccessRegistration(recv_json["username"].toString()));
            }
        }
        if(recv_json["type"] == MessageHelper::enumTypeToString(MessageHelper::TYPE::AddGroup)){
            if(!checkGroups(recv_json["namegroup"].toString())){
                sender_socket->write(MessageHelper::makeErrorGroup());
            }else{
                addGroup(recv_json["namegroup"].toString(), recv_json["username"].toString());
                sender_socket->write(MessageHelper::makeAddGroup(recv_json["namegroup"].toString(), recv_json["username"].toString()));

            }
        }
        if(recv_json["type"] == MessageHelper::enumTypeToString(MessageHelper::TYPE::AddUserInGroup)){
              addUserInGroup(recv_json["user"].toString(), recv_json["group"].toString());
              qDebug() << "ADD USER IN GROUP GET DATA: " << recv_json["user"].toString() << recv_json["group"].toString();
//            sender_socket->write(MessageHelper::makeAddUserInGroup(recv_json["user"].toString()));

              QJsonObject temp_obj = returnUserInGroup(recv_json["group"].toString());
                    qDebug() << "RETURNED FOR GROUP DATA JSON FROM DB: " << temp_obj;
              sender_socket->write(MessageHelper::makeGroupData(temp_obj));

        }

    }
}

QJsonObject Server::returnUserInGroup(const QString &group_name){
    QSqlQuery query_group, query_user;
    if(!query_group.exec("SELECT * FROM groups;") & !query_user.exec("SELECT * FROM users;")){
        qDebug() << "Unable groups to execut query.";
    }
    QSqlRecord rec_group = query_group.record();
    QSqlRecord rec_user = query_user.record();
    QString get_namegroup;
    int id_group = 0;
    QJsonObject json, groupname, user;
    QJsonArray arr_users, arr_group;
    while(query_group.next()){
        get_namegroup = query_group.value(rec_group.indexOf("group_name")).toString();
        if(get_namegroup == group_name) {
            id_group = query_group.value(rec_group.indexOf("id")).toInt();
        } else {
            qDebug() << "Group name is incorrect;";
        }
        groupname["namegroup"] = get_namegroup;

    }
    QString username;
    int user_id_group = 0;
    while(query_user.next()){
        user_id_group = query_user.value(rec_user.indexOf("group_id")).toInt();
        if(user_id_group == id_group){
            username = query_user.value(rec_user.indexOf("username")).toString();
            user["username"] = username;
            arr_users.append(user);
        }
    }
    groupname["users"] = arr_users;
    arr_group= json["group"].toArray();
    arr_group.append(groupname);
    json["group"] = arr_group;
    QJsonObject returned = json;
    return returned;
}

void Server::addUserInGroup(const QString &username, const QString& labelgroup){
    QSqlQuery query_group, query_users;
    if(!query_group.exec("SELECT * FROM groups;") & !query_users.exec("SELECT * FROM users;")){
        qDebug() << "Unable groups to execut query.";
    }
    QSqlRecord rec_group = query_group.record();
    QSqlRecord rec_user = query_users.record();
    QString str_group, str_user;
    int id_group;
    while(query_group.next()){
        str_group = query_group.value(rec_group.indexOf("group_name")).toString();
        id_group = query_group.value(rec_group.indexOf("id")).toInt();
    }
    qDebug() << "group id for added users: " << id_group;
    if(str_group == labelgroup){

        query_users.prepare("INSERT INTO users (username, group_id) VALUES (:username, :group_id)");
        query_users.bindValue(":username", username);
        query_users.bindValue(":group_id", id_group);
        query_users.exec();
    } else {
        qDebug() << "Group not found; Error";
    }


}

void Server::addGroup(const QString &namegroup, const QString& username){

    qDebug() << "group added.";

    //////////////SQL/////////////
    QSqlQuery query;
    query.prepare("INSERT INTO groups (group_name) VALUES (:group_name)");
    query.bindValue(":group_name",namegroup);
    query.exec();
    int group_id = query.lastInsertId().toInt();
    qDebug() << "Group_id: " << group_id;
    query.prepare("INSERT INTO users (username, group_id) VALUES(:username, :group_id)");
    query.bindValue(":username", username);
    query.bindValue(":group_id", group_id);
    query.exec();

      qDebug() << "Last Error: " <<  query.lastError();
    query.exec();

    ///////////////////////////

    QJsonObject json = doc_group.object();
    QJsonObject group, users;
    group["namegroup"] = namegroup;
    users["user"] = username;
    QJsonArray arr;
    arr.append(users);
    group["users"] = arr;
    QJsonArray groupArray = json["groups"].toArray();
    groupArray.append(group);
    json["groups"] = groupArray;
    doc_group.setObject(json);
    QFile file(group_file_path);
    if(file.open(QIODevice::WriteOnly)){
            file.write(doc_group.toJson(QJsonDocument::JsonFormat::Indented));
            file.close();
    }

}

void Server::deleteGroup(const QString &labelgroup) {


}

bool Server::checkUserinGroup(const QString &username){
    QFile file(group_file_path);
    QJsonObject json = doc_group.object();
    QJsonArray arr_json = json["groups"].toArray();
    for(const auto& var : arr_json){
        QJsonObject users_obj = var.toObject();
        QJsonArray array_user = users_obj.value("users").toArray();
        for(const auto& us : array_user){
            QJsonObject user_obj = us.toObject();
            if(user_obj.value("user").toString() == username){
                qCritical() << "this user is added in group< error";
                return false;
            }
        }
    }
    return true;
}
bool Server::checkGroups(const QString &namegroup){
    QFile file("groups.json");
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonObject json = doc.object();
    QJsonArray arr = json["groups"].toArray();
    for(int i = 0; i < arr.size(); ++i){
        QJsonObject temp = arr[i].toObject();
        if(temp.value("namegroup").toString() == namegroup){
            qDebug() << "Данная группа уже добавлена.";
            return false;
        }
    }
    qDebug() << "Данная группа успешно добавлена.";
    return true;
}


void Server::accountsBase(const QString &login, const QString &password, const QString &username) {
    QJsonObject json = users_doc.object();

    QJsonObject account;
    account["login"] = login;
    account["password"] = password;
    account["username"] = username;

    QJsonArray arr = json["accounts"].toArray();
    arr.append(account);
    json["accounts"] = arr;

    users_doc.setObject(json);

    QFile file(users_file_path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(users_doc.toJson(QJsonDocument::JsonFormat::Indented));
        file.close();
    }

    qDebug() << "данные получены";

    //////////////////////////////SQL//////////////////////////////
    QSqlQuery query;
    query.prepare("INSERT INTO account (login, password, username) VALUES (:login, :password, :username)");
    query.bindValue(":login", login);
    query.bindValue(":password", password);
    query.bindValue(":username", username);
    query.exec();
    qDebug() << "Account added in SQL base;\t";

}

bool Server::checkRegistration(const QString &login, const QString &username) {
    QJsonObject json = users_doc.object();
    QJsonArray arr = json["accounts"].toArray();
    for(int i = 0; i < arr.size(); ++i) {
        QJsonObject temp_value = arr[i].toObject();
        if(temp_value.value("login") == login) {
            qDebug() << "Данный логин уже зарегистрирован";
            return false;
        }
    }
    qDebug() << "Данный логин успешно зарегистрирован";
    return true;
}
void Server::sendToClient(QTcpSocket *clientSocket, const QJsonDocument& json) {
    qDebug() << " send data...";
    clientSocket->write(json.toBinaryData());
}
