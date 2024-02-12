#include "variant_m3u8.h"

#include <QDir>
#include <QLoggingCategory>

#include <mediaplaylist_m3u8.h>

Q_LOGGING_CATEGORY(variant_M3U8, "variant")

VariantM3U8::VariantM3U8(QObject *parent)
    : QObject{parent}
{
}

VariantM3U8::~VariantM3U8()
{
    if (m_reply) {
        m_reply->deleteLater();
        m_reply = nullptr;
    }
}

void VariantM3U8::setUrl(const QUrl &url)
{
    if (!url.isValid()) {
        qCDebug(variant_M3U8) << "set url:" << url << "is invalid, " << url.errorString();
        return;
    }

    m_url = url;
    m_effectiveUrl = m_url;

    m_file.setFileName(url.fileName());
    qCDebug(variant_M3U8) << "save to" << m_file.fileName();
}

void VariantM3U8::setHeader(const QByteArray &key, const QByteArray &value)
{
    m_requestHeaders.insert(key, value);
}

bool VariantM3U8::request()
{

    QNetworkRequest request(m_url);
    // TODO
    // request.setTransferTimeout(1000);

    auto it = m_requestHeaders.cbegin();
    while (it != m_requestHeaders.cend()) {
        request.setRawHeader(it.key(), it.value());
        qCDebug(variant_M3U8) << it.key() << ":" << it.value();
        ++it;
    }

    qCDebug(variant_M3U8) << "GET" << m_url.toString();

    m_reply = m_qnam.get(request);

    connect(m_reply, &QNetworkReply::finished, this, &VariantM3U8::replyFinished);
    connect(m_reply, &QIODevice::readyRead, this, &VariantM3U8::replyReadyRead);
    connect(m_reply, &QNetworkReply::sslErrors, this, &VariantM3U8::sslErrors);
    connect(m_reply, &QNetworkReply::errorOccurred, this, &VariantM3U8::processErrors);

    connect(m_reply, &QNetworkReply::downloadProgress, this, &VariantM3U8::networkReplyProgress);
    connect(m_reply, &QNetworkReply::redirected, this, &VariantM3U8::replyRedirected);

    if (!m_file.open(QIODevice::WriteOnly)) {
        qCDebug(variant_M3U8) << QString("Unable to save the file %1: %2.")
                                     .arg(QDir::toNativeSeparators(m_file.fileName()))
                                     .arg(m_file.errorString());
        replyFinished();
        return false;
    }

    return true;
}

void VariantM3U8::replyFinished()
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
        qCDebug(variant_M3U8) << "error:" << errorString;
    }

    qCDebug(variant_M3U8) << "finished:" << m_url;
    processVariantM3U8();
}

void VariantM3U8::replyReadyRead()
{
    m_file.write(m_reply->readAll());
}

void VariantM3U8::sslErrors(const QList<QSslError> &errors)
{
    QString errorString;
    for (const QSslError &error : errors) {
        if (!errorString.isEmpty())
            errorString += '\n';
        errorString += error.errorString();
    }

    qCDebug(variant_M3U8) << "TLS Errors, One or more TLS errors has occurred:\n" << errorString;
    m_reply->ignoreSslErrors();
}

void VariantM3U8::processErrors(QNetworkReply::NetworkError error)
{
    qCDebug(variant_M3U8) << "has error:" << error;
}

void VariantM3U8::networkReplyProgress(qint64 bytesRead, qint64 totalBytes)
{
    Q_UNUSED(bytesRead)
    Q_UNUSED(totalBytes)

    // TODO
    // add under command line option
    // qCDebug(variant_M3U8) << bytesRead << "/" << totalBytes;
}

void VariantM3U8::replyRedirected(const QUrl &url)
{
    qCDebug(variant_M3U8) << "redirected to" << url;
    m_effectiveUrl = url;
}

void VariantM3U8::mediaPlaylistFinished(const QUuid &uuid)
{
    if (!m_mediaPlaylistList.contains(uuid)) {
        qCDebug(variant_M3U8) << "unknown uuid: " << uuid;
        return;
    }

    const auto &mp = m_mediaPlaylistList.value(uuid);
    m_mediaPlaylistList.remove(mp->uuid());

    if (m_mediaPlaylistList.isEmpty()) {
        emit resultIsReady();
    }
}

void VariantM3U8::processVariantM3U8()
{
    QFileInfo fi(m_file.fileName());
    QFile playlist(fi.absoluteFilePath()); // variant.m3u8
    if (!playlist.open(QIODevice::ReadOnly)) {
        qCDebug(variant_M3U8) << QString("Unable to open the file %1: %2.")
                                     .arg(fi.absoluteFilePath())
                                     .arg(playlist.errorString());
        return;
    }

    QTextStream in(&playlist);
    while (!in.atEnd()) {
        const QString &line = in.readLine();

        if (line.startsWith("#EXT-X-STREAM-INF")) {
            const QString &mediaPlaylist = in.readLine();
            if (mediaPlaylist.startsWith("http")) {
                // absolute path to media playlist
                qCDebug(variant_M3U8) << "media playlist absolute path not supported";
                continue;
            }

            // relative path to media playlist
            QString url = m_effectiveUrl.toString();
            url.replace(m_url.fileName(), mediaPlaylist);

            QString targetDir
                = QString("%1/%2")
                      .arg(QDir::currentPath())
                      .arg(QUrl(mediaPlaylist)
                               .adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash)
                               .toString());
            if (targetDir.isEmpty()) {
                targetDir = QDir::currentPath();
            } else {
                if (!QDir().mkpath(targetDir)) {
                    qCDebug(variant_M3U8) << "create dir:" << targetDir << "failed";
                }
            }

            QSharedPointer<MediaPlaylistM3U8> mp(new MediaPlaylistM3U8(url, targetDir));
            auto it = m_requestHeaders.cbegin();
            while (it != m_requestHeaders.cend()) {
                mp->setHeader(it.key(), it.value());
                ++it;
            }

            m_mediaPlaylistList.insert(mp->uuid(), mp);
            connect(mp.get(), &MediaPlaylistM3U8::resultIsReady, this,
                    &VariantM3U8::mediaPlaylistFinished);

            mp->request();
        }
    }
}
