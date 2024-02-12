#ifndef MEDIAPLAYLIST_M3U8_H
#define MEDIAPLAYLIST_M3U8_H

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QUuid>

class Chunk;

class MediaPlaylistM3U8 : public QObject
{
    Q_OBJECT
public:
    explicit MediaPlaylistM3U8(const QUrl &url, const QString &dir, QObject *parent = nullptr);

    bool request();

    QUuid uuid() const;
    QString fileName() const;

    void setHeader(const QByteArray &key, const QByteArray &value);

signals:
    void resultIsReady(const QUuid &uuid);

private:
    void replyFinished();
    void replyReadyRead();
    void sslErrors(const QList<QSslError> &errors);
    void processErrors(QNetworkReply::NetworkError error);
    void replyRedirected(const QUrl &url);

    void processMediaPlaylist();
    void chunkFinished(const QUuid &chunkUuid);

    QUrl m_url;
    QUrl m_effectiveUrl;
    QHash<QByteArray, QByteArray> m_requestHeaders;

    QFile m_file;
    QUuid m_uuid; // playlist uuid

    QNetworkAccessManager m_qnam; // two instances in one thread, is OK ?
    QNetworkReply *m_reply = nullptr;

    QHash<QUuid, QSharedPointer<Chunk>> m_chunkListToDo;
    QHash<QUuid, QSharedPointer<Chunk>> m_chunkListInProgress;

    // TODO
    // get max number from OS and pass from cli
    const size_t m_maxOpenFiles = 20;
    QString m_dir;
};

#endif // MEDIAPLAYLIST_M3U8_H
