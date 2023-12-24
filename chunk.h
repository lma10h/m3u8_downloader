#ifndef CHUNK_H
#define CHUNK_H

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QUuid>

class Chunk : public QObject
{
    Q_OBJECT
public:
    explicit Chunk(const QUrl &url, QObject *parent = nullptr);

    bool request();

    QString fileName() const;
    QUuid uuid() const;

signals:
    void resultIsReady(const QUuid &uuid);

private:
    void replyFinished();
    void replyReadyRead();
    void sslErrors(const QList<QSslError> &errors);
    void processErrors(QNetworkReply::NetworkError error);

    QUrl m_url;
    QUuid m_uuid;
    QFile m_file;

    QNetworkAccessManager m_qnam; // two instances in one thread, is OK ?
    QNetworkReply *m_reply = nullptr;
};

#endif // CHUNK_H
