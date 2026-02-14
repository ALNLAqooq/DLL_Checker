#include "mainwindow.h"
#include "dependencyscanner.h"
#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QMetaType>
#include <QFile>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Register custom types for signal-slot communication
    qRegisterMetaType<QList<DependencyScanner::DependencyNode*>>("QList<DependencyScanner::DependencyNode*>");
    
    // Load translations - using QApplication's object tree for proper lifetime management
    QTranslator* translator = new QTranslator(&a);
    QString locale = QLocale::system().name();
    if (translator->load("dllchecker_" + locale, ":/translations")) {
        a.installTranslator(translator);
    }
    
    // Load and apply stylesheet
    QFile styleFile(":/style/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QString::fromUtf8(styleFile.readAll());
        a.setStyleSheet(style);
        styleFile.close();
    } else {
        qDebug() << "Warning: Could not load stylesheet from :/style/style.qss";
    }
    
    MainWindow w;
    w.show();
    return a.exec();
}
