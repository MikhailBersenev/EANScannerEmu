#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QFile>
#include <QTextStream>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Настройка сглаживания шрифтов для красивого рендеринга
    QFont defaultFont = a.font();
    defaultFont.setStyleStrategy(QFont::PreferQuality);
    defaultFont.setHintingPreference(QFont::PreferFullHinting);
    defaultFont.setKerning(true);
    a.setFont(defaultFont);

    // Загрузка и применение QSS стилей
    QFile styleFile(":/styles.qss");
    if (styleFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream styleStream(&styleFile);
        QString style = styleStream.readAll();
        a.setStyleSheet(style);
        styleFile.close();
    } else {
        // Попытка загрузить из файловой системы, если не в ресурсах
        QFile styleFileFS("styles.qss");
        if (styleFileFS.open(QFile::ReadOnly | QFile::Text)) {
            QTextStream styleStream(&styleFileFS);
            QString style = styleStream.readAll();
            a.setStyleSheet(style);
            styleFileFS.close();
        }
    }

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "EANScannerEmuSrc_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return a.exec();
}
