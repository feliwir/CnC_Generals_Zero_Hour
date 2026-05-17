#include "wdump.h"
#include "wdumpwin.h"
#include <QCommandLineParser>
#include <QFileOpenEvent>

WDumpApp::WDumpApp(int &argc, char **argv) : QApplication(argc, argv), DumpTextures(false), NoWindow(false), TextureDumpFile(nullptr)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("A UI tool to inspect Westwood 3D files");
    parser.addHelpOption();
    parser.addVersionOption();

    // A boolean option with a single name (-t)
    QCommandLineOption dumpTexturesOption("t", QCoreApplication::translate("main", "Dump texture usage to textures.txt"));
    parser.addOption(dumpTexturesOption);
    QCommandLineOption noWindowOption("q", QCoreApplication::translate("main", "Don't open a window"));
    parser.addOption(noWindowOption);

    parser.process(*this);

    DumpTextures = parser.isSet(dumpTexturesOption);
    NoWindow = parser.isSet(noWindowOption);

    // note: if any other dump types are enabled, they should probably open different files.
    if (DumpTextures)
    {
        TextureDumpFile = fopen("textures.txt", "a");
    }
}

bool WDumpApp::event(QEvent *event)
{
    // Handle custom events here if needed
    if (event->type() == QEvent::FileOpen)
    {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);
        const QUrl url = openEvent->url();
        if (url.isLocalFile())
        {
            // QFile localFile(url.toLocalFile());
            // read from local file
        }
        else
        {
            // parse openEvent->file()
        }
    }
    // Call the base class implementation for default processing
    return QApplication::event(event);
}

int main(int argc, char **argv)
{
    WDumpApp app(argc, argv);

    if (!app.NoWindow)
    {
        WDumpWindow *window = new WDumpWindow();
        window->resize(800, 600);
        window->show();
    }

    return app.exec();
}