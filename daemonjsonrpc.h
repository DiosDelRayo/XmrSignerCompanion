#ifndef DAEMONJSONRPC_H
#define DAEMONJSONRPC_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QAuthenticator>

class DaemonJsonRpc : public QObject
{
    Q_OBJECT

public:
    explicit DaemonJsonRpc(
        QObject *parent = nullptr,
        const QString& host = "localhost",
        int port = 18081,
        bool tls = false
        );
    void setAuthentication(const QString& username, const QString& password);
    int getHeight();
    int getLocalHeight();
    int getSynced(); // percentage 0...100 of the sync status
    QJsonObject getInfo();
    QString getVersion();

private:
    QJsonObject makeRequest(const QString &method, const QJsonObject &params);
    QString m_host;
    int m_port;
    bool m_tls;
    QNetworkAccessManager* m_networkManager;
    QUrl m_url;
    QString m_username;
    QString m_password;

private slots:
    void handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
};

#endif // DAEMONJSONRPC_H
