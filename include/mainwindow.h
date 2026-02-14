#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeWidget>
#include <QTextEdit>
#include <QProgressBar>
#include <QToolBar>
#include <QStatusBar>
#include <QCheckBox>
#include <QThread>
#include <memory>
#include "dependencyscanner.h"
#include "comparisonengine.h"
#include "scanworker.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onScanDirectory();
    void onScanSingleFile();
    void onImportMissingReport();
    void onExportMissingReport();
    void onExportReport();
    void onAutoCollectDLLs();
    void onClearAll();
    void onTreeItemClicked(QTreeWidgetItem* item, int column);
    void onScanProgress(int current, int total, const QString& file);
    void onScanFinished(QList<DependencyScanner::DependencyNode*> results);
    void onScanError(const QString& errorMessage);
    void onCancelScan();

private:
    void setupUI();
    void populateTree(DependencyScanner::DependencyNode* root);
    void showDLLDetails(QTreeWidgetItem* item);
    QTreeWidgetItem* createTreeItem(DependencyScanner::DependencyNode* node);
    void expandMissingNodes(QTreeWidgetItem* item);  // 自动展开缺失项
    bool hasMissingDependencies(QTreeWidgetItem* item);  // 检查是否包含缺失项
    void highlightMissingDLLs(const QStringList& missingDLLs);  // 高亮显示缺失DLL
    void highlightTreeItem(QTreeWidgetItem* item, const QStringList& missingDLLs, 
                          DependencyScanner::DependencyNode* node);  // 递归高亮
    QList<DependencyScanner::DependencyNode*> getHighlightedNodes();  // 获取高亮节点
    void clearAllData();  // 清空所有数据
    QString formatErrorWithSuggestion(const QString& errorMessage);  // 格式化错误信息并提供建议
    QString formatFileSize(qint64 bytes);  // 格式化文件大小
    
    QTreeWidget* m_treeWidget;
    QTextEdit* m_detailPanel;
    QProgressBar* m_progressBar;
    QToolBar* m_toolBar;
    QCheckBox* m_showSystemDLLs;
    QCheckBox* m_recursiveScan;

    QThread* m_scanThread;
    ScanWorker* m_scanWorker;
    QList<DependencyScanner::DependencyNode*> m_scanResults;
    QList<DependencyScanner::DependencyNode*> m_highlightedNodes;
    QMap<QTreeWidgetItem*, DependencyScanner::DependencyNode*> m_itemNodeMap;
    bool m_isScanning;
    bool m_isDestroying;
    qint64 m_scanStartTime;
    int m_scanLastCurrent;
    int m_scanLastTotal;
};

#endif // MAINWINDOW_H
