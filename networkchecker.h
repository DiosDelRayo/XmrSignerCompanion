#ifndef NETWORKCHECKER_H
#define NETWORKCHECKER_H

#include <QString>
#include <QHostAddress>
#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

class NetworkChecker
{
public:
    static bool isDomainOrIpLocal(const QString& host);
    static void checkMonerodRpc(const QString& address, std::function<void(bool)> callback);

private:
    static bool isIpLocal(const QHostAddress& address);
    static void onRpcReplyFinished(QNetworkReply *reply, std::function<void(bool)> callback);
};

#endif // NETWORKCHECKER_H
