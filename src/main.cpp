#include "cvcpelcod.h"

#include <QApplication>
#include <QStringList>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CVCPelcoD w;

    QStringList args = a.arguments();
    if (args.contains("-startUp")) {
        w.startup();
    }

    w.show();
    return a.exec();
}
