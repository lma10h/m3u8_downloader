#include <QCommandLineParser>
#include <QCoreApplication>

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QUrl>

#include <mediaplaylist_m3u8.h>
#include <variant_m3u8.h>

Q_LOGGING_CATEGORY(m3u8_d, "m3u8-d")

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("m3u8-downloader");
    QCoreApplication::setApplicationVersion("1.0");

    // QT_LOGGING_RULES="chunk*=true"
    QLoggingCategory::setFilterRules("chunk*=false");

    QCommandLineParser parser;
    parser.setApplicationDescription("restream, m3u8-downloader");
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption variantUrlOption(QStringList() << "va"
                                                            << "variant",
                                              "Set url to variant.m3u8",
                                              "http://example.com/variant.m3u8");
    parser.addOption(variantUrlOption);

    const QCommandLineOption mediaUrlOption(QStringList() << "mp"
                                                          << "media",
                                            "Set url to playlist.m3u8",
                                            "http://example.com/playlist.m3u8");
    parser.addOption(mediaUrlOption);

    const QCommandLineOption userAgentOption(QStringList() << "ua"
                                                           << "user-agent",
                                             "Set user agent", "\"Custom Agent\"");
    parser.addOption(userAgentOption);

    // process
    parser.process(app);

    int rcode = 0;
    if (parser.isSet(variantUrlOption)) {
        VariantM3U8 variant;
        QObject::connect(&variant, &VariantM3U8::resultIsReady, &app, &QCoreApplication::quit);

        // TODO
        // QUrl::fromEndoded() ?
        QUrl u(parser.value(variantUrlOption));
        if (!u.isValid()) {
            qCDebug(m3u8_d) << "invalid variant url:" << u.errorString();
            return 1;
        }

        variant.setUrl(parser.value(variantUrlOption));
        if (parser.isSet(userAgentOption)) {
            variant.setHeader("User-Agent", parser.value(userAgentOption).toUtf8());
        }

        variant.request();
        rcode = app.exec();
    } else if (parser.isSet(mediaUrlOption)) {
        MediaPlaylistM3U8 mediaPlaylist(parser.value(mediaUrlOption), QDir::currentPath());
        QObject::connect(&mediaPlaylist, &MediaPlaylistM3U8::resultIsReady, &app,
                         &QCoreApplication::quit);

        if (parser.isSet(userAgentOption)) {
            mediaPlaylist.setHeader("User-Agent", parser.value(userAgentOption).toUtf8());
        }

        mediaPlaylist.request();
        rcode = app.exec();
    }

    return rcode;
}
