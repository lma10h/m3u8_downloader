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
    explicit MediaPlaylistM3U8(const QUrl &url, QObject *parent = nullptr);

    bool request();

    QUuid uuid() const;
    QString fileName() const;

signals:
    void resultIsReady(const QUuid &uuid);

private:
    void replyFinished();
    void replyReadyRead();
    void sslErrors(const QList<QSslError> &errors);
    void processErrors(QNetworkReply::NetworkError error);
    void replyRedirected(const QUrl &url);

    void processMediaPlaylist();
    void chunkFinished(const QUuid &uuid);

    QUrl m_url;
    QUrl m_effectiveUrl;

    QFile m_file;
    QUuid m_uuid;

    QNetworkAccessManager m_qnam; // two instances in one thread, is OK ?
    QNetworkReply *m_reply = nullptr;

    QHash<QUuid, QSharedPointer<Chunk>> m_chunkList;
};

#endif // MEDIAPLAYLIST_M3U8_H
