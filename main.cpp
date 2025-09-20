#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QSettings settings("settings.ini", QSettings::IniFormat);

    if (settings.contains("Display/calibratedDPI"))
    {
        int ppi = settings.value("Display/calibratedDPI").toInt();
        if (ppi > 0)
        {  // only apply if not default
            qDebug() << "Setting Screen To PPI:" << ppi;
            qputenv("QT_FONT_DPI", QByteArray::number(ppi));
        } else
        {
            qDebug() << "Using system default DPI";
        }
    }

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
