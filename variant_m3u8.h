#pragma once

#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QUrl>

class MediaPlaylistM3U8;

class VariantM3U8 : public QObject
{
    Q_OBJECT
public:
    explicit VariantM3U8(QObject *parent = nullptr);
    ~VariantM3U8();

    void setUrl(const QUrl &url);
    void setHeader(const QByteArray &key, const QByteArray &value);

    bool request();

signals:
    void resultIsReady();

private:
    void replyFinished();
    void replyReadyRead();
    void sslErrors(const QList<QSslError> &errors);
    void processErrors(QNetworkReply::NetworkError error);

    void networkReplyProgress(qint64 bytesRead, qint64 totalBytes);
    void replyRedirected(const QUrl &url);

    void mediaPlaylistFinished(const QUuid &uuid);
    void processVariantM3U8();

    QUrl m_url;
    QUrl m_effectiveUrl;
    QHash<QByteArray, QByteArray> m_requestHeaders;

    QNetworkAccessManager m_qnam;
    QNetworkReply *m_reply = nullptr;
    QFile m_file;

    QHash<QUuid, QSharedPointer<MediaPlaylistM3U8>> m_mediaPlaylistList;
};
