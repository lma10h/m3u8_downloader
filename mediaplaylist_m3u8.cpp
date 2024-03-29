#include "mediaplaylist_m3u8.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QNetworkReply>
#include <QUrl>

#include <chunk.h>

Q_LOGGING_CATEGORY(media_M3U8, "media")

MediaPlaylistM3U8::MediaPlaylistM3U8(const QUrl &url, const QString &dir, QObject *parent)
    : QObject{parent}
    , m_url(url)
    , m_effectiveUrl(url)
    , m_file(QString("%1/%2").arg(dir).arg(m_url.fileName()))
    , m_uuid(QUuid::createUuid())
    , m_dir(dir)
{
    qCDebug(media_M3U8) << "save to" << m_file.fileName();

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
    qCDebug(media_M3U8) << "GET" << m_url.toString();

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

    qCDebug(media_M3U8) << "finished:" << m_url;
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
            url.replace(m_url.fileName(), chunkAddress);

            QSharedPointer<Chunk> chunk(new Chunk(url, m_dir));

            auto it = m_requestHeaders.cbegin();
            while (it != m_requestHeaders.cend()) {
                chunk->setHeader(it.key(), it.value());
                ++it;
            }

            m_chunkListToDo.insert(chunk->uuid(), chunk);
            connect(chunk.get(), &Chunk::resultIsReady, this, &MediaPlaylistM3U8::chunkFinished);
        }
    }

    // запустим @m_maxOpenFiles запросов одновременно
    // ограничение из за максимально возможного кол-ва открытых файлов

    qCDebug(media_M3U8) << "chunks:" << m_chunkListToDo.size();

    int number = m_maxOpenFiles;
    auto it = m_chunkListToDo.begin();
    while (number > 0) {
        if (it == m_chunkListToDo.end()) {
            break;
        }

        m_chunkListInProgress.insert(it.key(), it.value());
        m_chunkListToDo.remove(it.key());

        --number;
        ++it;
    }

    std::for_each(m_chunkListInProgress.begin(), m_chunkListInProgress.end(),
                  [](const QSharedPointer<Chunk> &p) { p->request(); });
}

void MediaPlaylistM3U8::chunkFinished(const QUuid &chunkUuid)
{
    if (!m_chunkListInProgress.contains(chunkUuid)) {
        qCDebug(media_M3U8) << "unknown uuid: " << chunkUuid;
        return;
    }

    m_chunkListInProgress.remove(chunkUuid);

    if (!m_chunkListToDo.isEmpty()) {
        auto nextChunk = m_chunkListToDo.begin();
        const auto nextUuid = nextChunk.key();
        const auto nextPointer = nextChunk.value();

        m_chunkListToDo.remove(nextUuid);
        m_chunkListInProgress.insert(nextUuid, nextPointer);

        nextPointer->request();
    }

    if (m_chunkListInProgress.isEmpty()) {
        emit resultIsReady(m_uuid);
        return;
    }
}
