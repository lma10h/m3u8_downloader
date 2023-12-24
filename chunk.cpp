#include "chunk.h"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QUuid>

Q_LOGGING_CATEGORY(chunk_M3U8, "chunk")

Chunk::Chunk(const QUrl &url, QObject *parent)
    : QObject{parent}
    , m_url(url)
    , m_uuid(QUuid::createUuid())
    , m_file(m_url.fileName())
{
    if (!m_url.isValid()) {
        qCDebug(chunk_M3U8) << "invalid url:" << url;
        throw std::invalid_argument("invalid url");
    }
}

bool Chunk::request()
{
    QNetworkRequest request(m_url);
    // TODO
    // request.setTransferTimeout(1000);

    auto it = m_requestHeaders.cbegin();
    while (it != m_requestHeaders.cend()) {
        request.setRawHeader(it.key(), it.value());
        qCDebug(chunk_M3U8) << it.key() << ":" << it.value();
        ++it;
    }

    qCDebug(chunk_M3U8) << "GET";

    m_reply = m_qnam.get(request);

    connect(m_reply, &QNetworkReply::finished, this, &Chunk::replyFinished);
    connect(m_reply, &QIODevice::readyRead, this, &Chunk::replyReadyRead);
    connect(m_reply, &QNetworkReply::sslErrors, this, &Chunk::sslErrors);
    connect(m_reply, &QNetworkReply::errorOccurred, this, &Chunk::processErrors);

    if (!m_file.open(QIODevice::WriteOnly)) {
        qCDebug(chunk_M3U8) << QString("Unable to save the file %1: %2.")
                                   .arg(QDir::toNativeSeparators(m_file.fileName()))
                                   .arg(m_file.errorString());
        replyFinished();
        return false;
    }

    return true;
}

QString Chunk::fileName() const
{
    return m_url.fileName();
}

QUuid Chunk::uuid() const
{
    return m_uuid;
}

void Chunk::setHeader(const QByteArray &key, const QByteArray &value)
{
    m_requestHeaders.insert(key, value);
}

void Chunk::replyFinished()
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
        qCDebug(chunk_M3U8) << "error:" << errorString;
    }

    emit resultIsReady(m_uuid);
}

void Chunk::replyReadyRead()
{
    m_file.write(m_reply->readAll());
}

void Chunk::sslErrors(const QList<QSslError> &errors)
{
    QString errorString;
    for (const QSslError &error : errors) {
        if (!errorString.isEmpty())
            errorString += '\n';
        errorString += error.errorString();
    }

    qCDebug(chunk_M3U8) << "TLS Errors, One or more TLS errors has occurred:\n" << errorString;
    m_reply->ignoreSslErrors();
}

void Chunk::processErrors(QNetworkReply::NetworkError error)
{
    qCDebug(chunk_M3U8) << "has error:" << error;
}
