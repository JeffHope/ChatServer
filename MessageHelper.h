#ifndef MESSAGEHELPER_H
#define MESSAGEHELPER_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaEnum>

class MessageHelper : public QObject {

public:
    MessageHelper() = default;
    ~MessageHelper() = default;

    enum STATUS {
        Offline = 0x00,
        Online = 0x01,
        Idle = 0x02,
    };
    Q_ENUM(STATUS)

    enum TYPE {
        Message = 0x00,
        Status = 0x01,
        Broadcast = 0x02,
        Auth = 0x03,
        Access = 0x04,
        DoubleSigned = 0x05,
        Registration = 0x06,
        RegistrationAccess = 0x2f,
        RegistrationError = 0x3f,
        AddGroup = 0x4a,
        ErrorGroup = 0x4b,
        AccessGroup = 0x4c,
        MessageGroup = 0x4d,
        AddUserInGroup = 0x4e,
        Error = 0xfe,
    };
    Q_ENUM(TYPE)

    static QByteArray make(TYPE type, const QJsonObject &object) {
        QJsonDocument doc;
        QJsonObject json;
        json["type"] = enumTypeToString(type);
        for (const auto &key : object.keys()) {
            json[key] = object[key];
        }
        doc.setObject(json);
        return doc.toBinaryData();
    }

    static QByteArray makeGroupData(QJsonObject obj){
        QJsonObject json = obj;
        return make(TYPE::AddUserInGroup, json);
    }

    static QByteArray makeMessage(const QString &username, const QString &msg) {
        QJsonObject json;
        json["username"] = username;
        json["message"] = msg;
        return make(TYPE::Message, json);
    }

    static QByteArray makeStatus(const QString &username, const STATUS &status) {
        QJsonObject json;
        json["username"] = username;
        json["status"] = enumStatusToString(status);
        return make(TYPE::Status, json);
    }

    static QByteArray makeAuth(const QString &login, const QString &password) {
        QJsonObject json;
        json["login"] = login;
        json["password"] = password;
        return make(TYPE::Auth, json);
    }

    static QByteArray makeAccess(const QString &username) {
        QJsonObject json;
        json["username"] = username;
        return make(TYPE::Access, json);
    }

    static QByteArray makeError(const QString &reason = "") {
        QJsonObject json;
        json["reason"] = reason;
        return make(TYPE::Error, json);
    }

    static QByteArray makeErrorRegistration(const QString &cause = " "){
        QJsonObject json;
        json["cause"] = cause;
        return make(TYPE::RegistrationError, json);
    }

    static QByteArray makeAccessRegistration(const QString &username){
        QJsonObject json;
        json["username"] = username;
        return make(TYPE::RegistrationAccess, json);
    }
    static QByteArray makeDoubleSignError(const QString &doubleSign = ""){
        QJsonObject json;
        json["doubleSign"] = doubleSign;
        return make(TYPE::DoubleSigned, json);
    }

    static QByteArray makeRegistration(const QString& login, const QString& password, const QString& username){
        QJsonObject json;
        json["login"] = login;
        json["password"] = password;
        json["username"] = username;
        return make(TYPE::Registration, json);
    }
    static QByteArray makeAddGroup(const QString& namegroup, const QString& username){
        QJsonObject json;
        json["namegroup"] = namegroup;
        json["username"] = username;
        return make(TYPE::AddGroup, json);
    }
    static QByteArray makeErrorGroup(const QString& cause = " "){
        QJsonObject json;
        json["cause"] = cause;
        return make(TYPE::ErrorGroup, json);
    }
    static QByteArray makeAccessGroup(const QString& namegroup){
        QJsonObject json;
        json["namegroup"] = namegroup;
        return make(TYPE::AccessGroup, json);
    }
    static QByteArray makeMessageGroup(const QString &username, const QString &msg) {
        QJsonObject json;
        json["username"] = username;
        json["message"] = msg;
        return make(TYPE::MessageGroup, json);
    }

    static QByteArray makeAddUserInGroup(const QString &username, const QString & labelgroup) {
        QJsonObject json;
        json["user"] = username;
        json["group"] = labelgroup;
        return make(TYPE::AddUserInGroup, json);
    }


    template<typename QEnum>
    static QString QtEnumString(const QEnum &value) {
        return QString(QMetaEnum::fromType<QEnum>().valueToKey(value));
    }

    static QString enumStatusToString(STATUS value) {
        QString str = "";
        switch (value)
        {
        case STATUS::Offline:
            str = "Offline";
            break;
        case STATUS::Online:
            str = "Online";
            break;
        case STATUS::Idle:
            str = "Idle";
            break;
        default:
            break;
        }
        return str;
    }

    static QString enumTypeToString(TYPE value) {
        QString str = "";
        switch (value) {
        case TYPE::Message:
            str = "Message";
            break;
        case TYPE::Status:
            str = "Status";
            break;
        case TYPE::Broadcast:
            str = "Broadcast";
            break;
        case TYPE::Auth:
            str = "Auth";
            break;
        case TYPE::Access:
            str = "Access";
            break;
        case TYPE::Error:
            str = "Error";
            break;
        case TYPE::DoubleSigned:
            str = "DoubleSigned";
            break;
        case TYPE::Registration:
            str = "Registration";
            break;
        case TYPE::RegistrationAccess:
            str = "RegistrationAccess";
            break;
        case TYPE::RegistrationError:
            str = "RegistrationError";
            break;
        case TYPE::AddGroup:
            str = "AddGroup";
            break;
        case TYPE::ErrorGroup:
            str = "ErrorGroup";
            break;
        case TYPE::AccessGroup:
            str = "AccessGroup";
            break;
        case TYPE::MessageGroup:
            str = "MessageGroup";
            break;
        case TYPE::AddUserInGroup:
            str = "AddUserInGroup";
            break;
        default:
            break;
        }
        return str;
    }

    static STATUS statusStringToEnum(const QString &value) {
        if (value == "Online") {
            return STATUS::Online;
        } else if (value == "Idle") {
            return STATUS::Idle;
        }
        return STATUS::Offline;
    }

};

#endif /* MESSAGEHELPER_H */
