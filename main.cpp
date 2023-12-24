#include <QCommandLineParser>
#include <QCoreApplication>

#include <QFileInfo>
#include <QLoggingCategory>
#include <QUrl>

#include <variant_m3u8.h>

Q_LOGGING_CATEGORY(m3u8_d, "m3u8-d")

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    QCoreApplication::setApplicationName("m3u8-downloader");
    QCoreApplication::setApplicationVersion("1.0");

    QCommandLineParser parser;
    parser.setApplicationDescription("restream, m3u8-downloader");
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption variantUrlOption(QStringList() << "va"
                                                            << "variant",
                                              "Set url to variant.m3u8",
                                              "http://example.com/variant.m3u8");
    parser.addOption(variantUrlOption);

    const QCommandLineOption userAgentOption(QStringList() << "ua"
                                                           << "user-agent",
                                             "Set user agent", "\"Custom Agent\"");
    parser.addOption(userAgentOption);

    // process
    parser.process(app);

    VariantM3U8 variant;
    QObject::connect(&variant, &VariantM3U8::resultIsReady, &app, &QCoreApplication::quit);

    if (parser.isSet(variantUrlOption)) {
        // TODO
        // QUrl::fromEndoded() ?
        QUrl u(parser.value(variantUrlOption));
        if (!u.isValid()) {
            qCDebug(m3u8_d) << "invalid variant url:" << u.errorString();
            return 1;
        }

        variant.setUrl(u);
    }

    if (parser.isSet(userAgentOption)) {
        variant.setHeader("User-Agent", parser.value(userAgentOption).toUtf8());
    }

    variant.request();

    return app.exec();
}
