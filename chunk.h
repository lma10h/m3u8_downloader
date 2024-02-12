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
    explicit Chunk(const QUrl &url, const QString &dir, QObject *parent = nullptr);

    bool request();

    QString fileName() const;
    QUuid uuid() const;

    void setHeader(const QByteArray &key, const QByteArray &value);

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
    QHash<QByteArray, QByteArray> m_requestHeaders;

    QNetworkAccessManager m_qnam; // two instances in one thread, is OK ?
    QNetworkReply *m_reply = nullptr;

    QString m_dir;
};

#endif // CHUNK_H
