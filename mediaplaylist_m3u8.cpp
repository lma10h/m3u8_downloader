#include "mediaplaylist_m3u8.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QUrl>

#include <chunk.h>

Q_LOGGING_CATEGORY(media_M3U8, "media")

MediaPlaylistM3U8::MediaPlaylistM3U8(const QUrl &url, QObject *parent)
    : QObject{parent}
    , m_url(url)
    , m_effectiveUrl(url)
    , m_file(m_url.fileName())
    , m_uuid(QUuid::createUuid())
{
    if (!url.isValid()) {
        qCDebug(media_M3U8) << "invalid url:" << url;
        throw std::invalid_argument("invalid url");
    }
}

bool MediaPlaylistM3U8::request()
{
    QNetworkRequest request(m_url);
    // TODO
    // request.setTransferTimeout(1000);

    auto it = m_requestHeaders.cbegin();
    while (it != m_requestHeaders.cend()) {
        request.setRawHeader(it.key(), it.value());
        qCDebug(media_M3U8) << it.key() << ":" << it.value();
        ++it;
    }
    qCDebug(media_M3U8) << "GET";

    m_reply = m_qnam.get(request);

    connect(m_reply, &QNetworkReply::finished, this, &MediaPlaylistM3U8::replyFinished);
    connect(m_reply, &QIODevice::readyRead, this, &MediaPlaylistM3U8::replyReadyRead);
    connect(m_reply, &QNetworkReply::sslErrors, this, &MediaPlaylistM3U8::sslErrors);
    connect(m_reply, &QNetworkReply::errorOccurred, this, &MediaPlaylistM3U8::processErrors);
    connect(m_reply, &QNetworkReply::redirected, this, &MediaPlaylistM3U8::replyRedirected);

    if (!m_file.open(QIODevice::WriteOnly)) {
        qCDebug(media_M3U8) << QString("Unable to save the file %1: %2.")
                                   .arg(QDir::toNativeSeparators(m_file.fileName()))
                                   .arg(m_file.errorString());
        replyFinished();
        return false;
    }

    return true;
}

QUuid MediaPlaylistM3U8::uuid() const
{
    return m_uuid;
}

QString MediaPlaylistM3U8::fileName() const
{
    return m_effectiveUrl.fileName();
}

void MediaPlaylistM3U8::setHeader(const QByteArray &key, const QByteArray &value)
{
    m_requestHeaders.insert(key, value);
}

void MediaPlaylistM3U8::replyFinished()
{
    m_file.close();

    QNetworkReply::NetworkError error = m_reply->error();
    const QString &errorString = m_reply->errorString();
    m_reply->deleteLater();
    m_reply = nullptr;

    if (error != QNetworkReply::NoError) {
        QFileInfo fi;
        fi.setFile(m_file.fileName());

        QFile::remove(fi.absoluteFilePath());
        qCDebug(media_M3U8) << "error:" << errorString;
    }

    processMediaPlaylist();
}

void MediaPlaylistM3U8::replyReadyRead()
{
    m_file.write(m_reply->readAll());
}

void MediaPlaylistM3U8::sslErrors(const QList<QSslError> &errors)
{
    QString errorString;
    for (const QSslError &error : errors) {
        if (!errorString.isEmpty())
            errorString += '\n';
        errorString += error.errorString();
    }

    qCDebug(media_M3U8) << "TLS Errors, One or more TLS errors has occurred:\n" << errorString;
    m_reply->ignoreSslErrors();
}

void MediaPlaylistM3U8::processErrors(QNetworkReply::NetworkError error)
{
    qCDebug(media_M3U8) << "has error:" << error;
}

void MediaPlaylistM3U8::replyRedirected(const QUrl &url)
{
    qCDebug(media_M3U8) << "redirected to" << url;
    m_effectiveUrl = url;
}

void MediaPlaylistM3U8::processMediaPlaylist()
{
    QFileInfo fi(m_file.fileName());
    QFile playlist(fi.absoluteFilePath());
    if (!playlist.open(QIODevice::ReadOnly)) {
        qCDebug(media_M3U8) << QString("Unable to open the file %1: %2.")
                                   .arg(fi.absoluteFilePath())
                                   .arg(playlist.errorString());
        return;
    }

    QTextStream in(&playlist);
    while (!in.atEnd()) {
        const QString &line = in.readLine();
        if (line.startsWith("#EXTINF")) {
            const QString &chunkAddress = in.readLine();
            if (chunkAddress.startsWith("http")) {
                // absolute path to media playlist
                qCDebug(media_M3U8) << "chunk absolute path not supported";
                continue;
            }

            // relative path to media playlist
            QString url = m_effectiveUrl.toString();
            url.replace(m_file.fileName(), chunkAddress);

            QSharedPointer<Chunk> chunk(new Chunk(url));
            auto it = m_requestHeaders.cbegin();
            while (it != m_requestHeaders.cend()) {
                chunk->setHeader(it.key(), it.value());
                ++it;
            }

            m_chunkList.insert(chunk->uuid(), chunk);
            connect(chunk.get(), &Chunk::resultIsReady, this, &MediaPlaylistM3U8::chunkFinished);

            chunk->request();
        }
    }
}

void MediaPlaylistM3U8::chunkFinished(const QUuid &uuid)
{
    if (!m_chunkList.contains(uuid)) {
        qCDebug(media_M3U8) << "unknown uuid: " << uuid;
        return;
    }

    auto chunk = m_chunkList.value(uuid);
    qCDebug(media_M3U8) << "chunk:" << chunk->fileName() << "finished";
    m_chunkList.remove(chunk->uuid());

    if (m_chunkList.isEmpty()) {
        emit resultIsReady(m_uuid);
    }
}
