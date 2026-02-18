#include "mainwindow.h"
#include "dependencyscanner.h"
#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QMetaType>
#include <QFile>
#include <QDebug>
#include <QMessageBox>
#include <QDir>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // Initialize resources
    Q_INIT_RESOURCE(resources);
    
    // Debug: List all available resources
    QDir resourceDir(":/");
    qDebug() << "Root resources:" << resourceDir.entryList();
    QDir iconsDir(":/icons");
    qDebug() << "Icons resources:" << iconsDir.entryList();
    
    // Register custom types for signal-slot communication
    qRegisterMetaType<DependencyScanner::NodePtr>("DependencyScanner::NodePtr");
    qRegisterMetaType<QList<DependencyScanner::NodePtr>>("QList<DependencyScanner::NodePtr>");
    
    // Load translations - using QApplication's object tree for proper lifetime management
    QTranslator* translator = new QTranslator(&a);
    QString locale = QLocale::system().name();
    if (translator->load("dllchecker_" + locale, ":/translations")) {
        a.installTranslator(translator);
    }
    
    // Create main window first
    MainWindow w;
    w.show();
    
    // Load and apply stylesheet
    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QString::fromUtf8(styleFile.readAll());
        a.setStyleSheet(style);
        styleFile.close();
        qDebug() << "Stylesheet loaded successfully";
    } else {
        qDebug() << "Warning: Could not load stylesheet from :/style.qss";
        // Apply a basic fallback stylesheet
        a.setStyleSheet(
            "QMainWindow { background-color: #f5f7fa; }"
            "QTreeWidget { background-color: #ffffff; border: 1px solid #e5e7eb; border-radius: 4px; }"
            "QTreeWidget::item { padding: 4px 8px; min-height: 28px; }"
            "QTreeWidget::item:hover { background-color: #f9fafb; }"
            "QTreeWidget::item:selected { background-color: #dbeafe; color: #1e40af; }"
            "QToolBar { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #ffffff, stop:1 #f0f2f5); border-bottom: 1px solid #d1d5db; }"
            "QToolBar QToolButton { background-color: transparent; border: none; padding: 6px 12px; border-radius: 4px; }"
            "QToolBar QToolButton:hover { background-color: #e5e7eb; }"
            "QCheckBox { spacing: 6px; }"
            "QCheckBox::indicator { width: 16px; height: 16px; border-radius: 3px; border: 2px solid #9ca3af; background-color: #ffffff; }"
            "QCheckBox::indicator:checked { background-color: #3b82f6; border-color: #3b82f6; }"
            "QTextEdit { background-color: #ffffff; border: 1px solid #e5e7eb; border-radius: 4px; padding: 12px; }"
            "QStatusBar { background-color: #ffffff; border-top: 1px solid #e5e7eb; }"
            "QProgressBar { background-color: #f3f4f6; border: none; border-radius: 3px; height: 18px; }"
            "QProgressBar::chunk { background-color: #3b82f6; border-radius: 3px; }"
        );
        
        // Show user-friendly warning with proper parent window
        // Check if window is valid to prevent crash if MainWindow initialization failed
        if (w.isVisible()) {
            QMessageBox::warning(&w, QObject::tr("样式表加载失败"),
                QObject::tr("无法加载自定义样式表，将使用默认样式。\n\n"
                          "这可能是因为资源文件未正确编译到程序中。\n"
                          "请确保使用CMake重新编译项目，并且resources.qrc文件已正确配置。\n\n"
                          "程序仍可正常运行，但界面可能显示为默认样式。"));
        } else {
            QMessageBox::warning(nullptr, QObject::tr("样式表加载失败"),
                QObject::tr("无法加载自定义样式表，将使用默认样式。\n\n"
                          "这可能是因为资源文件未正确编译到程序中。\n"
                          "请确保使用CMake重新编译项目，并且resources.qrc文件已正确配置。\n\n"
                          "程序仍可正常运行，但界面可能显示为默认样式。"));
        }
    }
    
    return a.exec();
}
