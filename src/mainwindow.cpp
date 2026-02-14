#include "mainwindow.h"
#include "reportgenerator.h"
#include "dllcollector.h"
#include "inputvalidator.h"
#include "logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QAction>
#include <QCheckBox>
#include <QProgressDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDesktopServices>
#include <QUrl>
#include <QPointer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_scanThread(nullptr)
    , m_scanWorker(nullptr)
    , m_isScanning(false)
    , m_isDestroying(false)
    , m_scanStartTime(0)
    , m_scanLastCurrent(0)
    , m_scanLastTotal(0)
{
    setupUI();
}

MainWindow::~MainWindow()
{
    m_isDestroying = true;
    
    if (m_scanWorker) {
        m_scanWorker->cancel();
    }
    
    if (m_scanThread) {
        if (m_scanThread->isRunning()) {
            m_scanThread->quit();
            if (!m_scanThread->wait(5000)) {
                m_scanThread->terminate();
                m_scanThread->wait(1000);
            }
        }
        m_scanThread = nullptr;
    }
    
    m_scanWorker = nullptr;
    clearAllData();
}

void MainWindow::setupUI()
{
    setWindowTitle(tr("DLL依赖检查工具"));
    resize(1200, 800);
    
    // Create central widget
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);
    
    // Create toolbar
    m_toolBar = addToolBar(tr("主工具栏"));
    m_toolBar->setMovable(false);
    m_toolBar->setFloatable(false);
    m_toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    
    QAction* scanDirAction = m_toolBar->addAction(QIcon(":/icons/folder.svg"), tr("扫描文件夹"));
    scanDirAction->setToolTip(tr("扫描整个文件夹中的DLL依赖关系"));
    connect(scanDirAction, &QAction::triggered, this, &MainWindow::onScanDirectory);
    
    QAction* scanFileAction = m_toolBar->addAction(QIcon(":/icons/file.svg"), tr("扫描文件"));
    scanFileAction->setToolTip(tr("扫描单个文件的DLL依赖关系"));
    connect(scanFileAction, &QAction::triggered, this, &MainWindow::onScanSingleFile);
    
    m_toolBar->addSeparator();
    
    QAction* importReportAction = m_toolBar->addAction(QIcon(":/icons/import.svg"), tr("导入差异报告"));
    importReportAction->setToolTip(tr("导入目标机的缺失DLL报告"));
    connect(importReportAction, &QAction::triggered, this, &MainWindow::onImportMissingReport);
    
    QAction* exportMissingAction = m_toolBar->addAction(QIcon(":/icons/export.svg"), tr("导出缺失报告"));
    exportMissingAction->setToolTip(tr("导出当前扫描结果的缺失DLL报告"));
    connect(exportMissingAction, &QAction::triggered, this, &MainWindow::onExportMissingReport);
    
    // 隐藏旧的导出报告功能（新工作流程不需要）
    // QAction* exportAction = m_toolBar->addAction(tr("导出报告"));
    // connect(exportAction, &QAction::triggered, this, &MainWindow::onExportReport);
    
    m_toolBar->addSeparator();
    
    QAction* autoCollectAction = m_toolBar->addAction(QIcon(":/icons/download.svg"), tr("一键补齐"));
    autoCollectAction->setToolTip(tr("自动收集并复制高亮的缺失DLL"));
    connect(autoCollectAction, &QAction::triggered, this, &MainWindow::onAutoCollectDLLs);
    
    m_toolBar->addSeparator();
    
    QAction* clearAllAction = m_toolBar->addAction(QIcon(":/icons/clear.svg"), tr("一键清空"));
    clearAllAction->setToolTip(tr("清空所有扫描结果"));
    connect(clearAllAction, &QAction::triggered, this, &MainWindow::onClearAll);
    
    m_toolBar->addSeparator();
    
    // Add checkbox for showing system DLLs
    m_showSystemDLLs = new QCheckBox(tr("显示系统DLL"), this);
    m_showSystemDLLs->setChecked(false);
    m_showSystemDLLs->setToolTip(tr("是否显示系统DLL文件"));
    m_toolBar->addWidget(m_showSystemDLLs);
    
    // Add checkbox for recursive scanning
    m_recursiveScan = new QCheckBox(tr("递归扫描"), this);
    m_recursiveScan->setChecked(true);
    m_recursiveScan->setToolTip(tr("是否递归扫描子文件夹"));
    m_toolBar->addWidget(m_recursiveScan);

    m_toolBar->addSeparator();

    QAction* cancelAction = m_toolBar->addAction(QIcon(":/icons/cancel.svg"), tr("取消扫描"));
    cancelAction->setEnabled(false);
    cancelAction->setToolTip(tr("取消当前正在进行的扫描"));
    connect(cancelAction, &QAction::triggered, this, &MainWindow::onCancelScan);

    cancelAction->setObjectName("cancelAction");
    
    // Create main layout
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    
    // Create splitter for tree and details
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Create tree widget
    m_treeWidget = new QTreeWidget(this);
    m_treeWidget->setHeaderLabels(QStringList() << tr("文件") << tr("路径") 
                                                << tr("架构") << tr("版本") 
                                                << tr("状态"));
    m_treeWidget->setColumnWidth(0, 250);
    m_treeWidget->setColumnWidth(1, 350);
    m_treeWidget->setAnimated(true);
    m_treeWidget->setIndentation(20);
    m_treeWidget->setRootIsDecorated(true);
    m_treeWidget->setStyleSheet("");
    connect(m_treeWidget, &QTreeWidget::itemClicked,
            this, &MainWindow::onTreeItemClicked);
    
    // Create detail panel
    m_detailPanel = new QTextEdit(this);
    m_detailPanel->setReadOnly(true);
    
    splitter->addWidget(m_treeWidget);
    splitter->addWidget(m_detailPanel);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(splitter);
    
    // Create status bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setVisible(false);
    statusBar()->addPermanentWidget(m_progressBar);
    statusBar()->showMessage(tr("就绪"));
}

void MainWindow::onScanDirectory()
{
    QString dirPath = QFileDialog::getExistingDirectory(this, tr("选择要扫描的文件夹"));
    if (dirPath.isEmpty()) {
        return;
    }

    if (m_isScanning) {
        QMessageBox::information(this, tr("正在扫描"), tr("正在扫描中，请稍候..."));
        return;
    }

    // 验证目录路径
    InputValidator::ValidationResult validation = InputValidator::validateDirectoryPath(dirPath, false);
    if (validation != InputValidator::Valid) {
        LOG_WARNING("MainWindow", QString("目录验证失败: %1 - %2").arg(dirPath).arg(InputValidator::getErrorMessage(validation)));
        QMessageBox::warning(this, tr("路径验证失败"), 
            tr("无法扫描此目录：\n%1\n\n原因：%2")
            .arg(dirPath)
            .arg(InputValidator::getErrorMessage(validation)));
        return;
    }

    statusBar()->showMessage(tr("正在扫描文件夹: %1").arg(dirPath));
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 0);

    m_treeWidget->clear();
    m_itemNodeMap.clear();
    qDeleteAll(m_scanResults);
    m_scanResults.clear();

    m_isScanning = true;
    m_scanStartTime = QDateTime::currentMSecsSinceEpoch();
    m_scanLastCurrent = 0;
    m_scanLastTotal = 0;

    if (m_scanThread) {
        m_scanThread->deleteLater();
        m_scanThread = nullptr;
    }

    m_scanThread = new QThread();
    m_scanWorker = new ScanWorker();
    m_scanWorker->moveToThread(m_scanThread);

    connect(m_scanThread, &QThread::finished, m_scanWorker, &QObject::deleteLater);
    connect(m_scanThread, &QThread::finished, m_scanThread, &QObject::deleteLater);
    connect(m_scanWorker, &ScanWorker::scanProgress, this, &MainWindow::onScanProgress);
    connect(m_scanWorker, &ScanWorker::scanFinished, this, &MainWindow::onScanFinished);
    connect(m_scanWorker, &ScanWorker::scanError, this, &MainWindow::onScanError);

    QAction* cancelAction = m_toolBar->findChild<QAction*>("cancelAction");
    if (cancelAction) {
        cancelAction->setEnabled(true);
    }

    m_scanThread->start();

    auto weakThis = QPointer<MainWindow>(this);
    auto scanWorker = m_scanWorker;
    auto showSystemDLLs = m_showSystemDLLs->isChecked();
    auto recursive = m_recursiveScan->isChecked();

    QMetaObject::invokeMethod(scanWorker, [weakThis, scanWorker, dirPath, showSystemDLLs, recursive]() {
        if (!weakThis.isNull()) {
            scanWorker->scanDirectory(dirPath, recursive, showSystemDLLs);
        }
    }, Qt::QueuedConnection);
}

void MainWindow::onScanSingleFile()
{
    QString filePath = QFileDialog::getOpenFileName(this, tr("选择要扫描的文件"),
                                                    QString(), tr("可执行文件 (*.exe *.dll)"));
    if (filePath.isEmpty()) {
        return;
    }

    if (m_isScanning) {
        QMessageBox::information(this, tr("正在扫描"), tr("正在扫描中，请稍候..."));
        return;
    }

    statusBar()->showMessage(tr("正在扫描文件: %1").arg(filePath));
    m_progressBar->setVisible(true);
    m_progressBar->setRange(0, 1);
    m_progressBar->setValue(0);

    m_treeWidget->clear();
    m_itemNodeMap.clear();
    qDeleteAll(m_scanResults);
    m_scanResults.clear();

    m_isScanning = true;
    m_scanStartTime = QDateTime::currentMSecsSinceEpoch();
    m_scanLastCurrent = 0;
    m_scanLastTotal = 0;

    if (m_scanThread) {
        m_scanThread->deleteLater();
        m_scanThread = nullptr;
    }

    m_scanThread = new QThread();
    m_scanWorker = new ScanWorker();
    m_scanWorker->moveToThread(m_scanThread);

    connect(m_scanThread, &QThread::finished, m_scanWorker, &QObject::deleteLater);
    connect(m_scanThread, &QThread::finished, m_scanThread, &QObject::deleteLater);
    connect(m_scanWorker, &ScanWorker::scanProgress, this, &MainWindow::onScanProgress);
    connect(m_scanWorker, &ScanWorker::scanFinished, this, &MainWindow::onScanFinished);
    connect(m_scanWorker, &ScanWorker::scanError, this, &MainWindow::onScanError);

    QAction* cancelAction = m_toolBar->findChild<QAction*>("cancelAction");
    if (cancelAction) {
        cancelAction->setEnabled(true);
    }

    m_scanThread->start();

    auto weakThis = QPointer<MainWindow>(this);
    auto scanWorker = m_scanWorker;
    auto showSystemDLLs = m_showSystemDLLs->isChecked();

    QMetaObject::invokeMethod(scanWorker, [weakThis, scanWorker, filePath, showSystemDLLs]() {
        if (!weakThis.isNull()) {
            scanWorker->scanFile(filePath, showSystemDLLs);
        }
    }, Qt::QueuedConnection);
}

void MainWindow::onImportMissingReport()
{
    // 步骤1: 检查是否已经扫描了开发机
    if (m_scanResults.isEmpty()) {
        QMessageBox::warning(this, tr("请先扫描"), 
            tr("请先在开发机上扫描文件夹，然后再导入目标机的缺失报告。\n\n"
               "建议：\n"
               "1. 点击【扫描文件夹】或【扫描文件】按钮\n"
               "2. 选择要扫描的文件或文件夹\n"
               "3. 等待扫描完成后再导入报告"));
        return;
    }
    
    // 步骤2: 选择缺失报告文件
    QString reportPath = QFileDialog::getOpenFileName(this, tr("选择缺失报告"),
                                                    QString(), tr("JSON 文件 (*.json)"));
    if (reportPath.isEmpty()) {
        return;
    }
    
    // 步骤3: 加载缺失报告
    ComparisonEngine::MissingReport report = ComparisonEngine::loadMissingReport(reportPath);
    if (report.missingDLLs.isEmpty()) {
        QMessageBox::warning(this, tr("导入失败"), 
            tr("报告中没有缺失的DLL，或者报告文件格式错误。\n\n"
               "建议：\n"
               "1. 确认选择的是正确的JSON报告文件\n"
               "2. 检查报告文件是否完整\n"
               "3. 重新在目标机上导出缺失报告"));
        return;
    }
    
    statusBar()->showMessage(tr("缺失报告已加载，正在查找并高亮显示..."));
    
    // 步骤4: 在依赖树中查找缺失的DLL
    m_highlightedNodes = ComparisonEngine::findMissingDLLsInTree(m_scanResults, report);
    
    if (m_highlightedNodes.isEmpty()) {
        QMessageBox::warning(this, tr("未找到"), 
            tr("在当前扫描结果中未找到报告中列出的缺失DLL。\n\n"
               "可能的原因：\n"
               "1. 在开发机上扫描了错误的文件夹\n"
               "2. 目标机和开发机的DLL版本或路径不同\n"
               "3. 报告文件可能来自不同的环境\n\n"
               "建议：\n"
               "1. 确认在开发机上扫描了正确的文件夹\n"
               "2. 重新在目标机上生成缺失报告\n"
               "3. 确保两台机器使用相同的软件版本"));
        return;
    }
    
    // 步骤5: 高亮显示缺失的DLL
    highlightMissingDLLs(report.missingDLLs);
    
    // 步骤6: 显示结果（统计唯一的DLL名称）
    QSet<QString> uniqueDLLs;
    for (const auto* node : m_highlightedNodes) {
        uniqueDLLs.insert(node->fileName);
    }
    
    QString resultText = tr("已找到并高亮显示 %1 个缺失的DLL（共 %2 处位置）:\n\n")
        .arg(uniqueDLLs.size())
        .arg(m_highlightedNodes.size());
    
    int count = 0;
    QStringList displayedDLLs;
    for (const auto* node : m_highlightedNodes) {
        if (displayedDLLs.contains(node->fileName)) {
            continue; // 跳过重复的DLL
        }
        if (count >= 10) break;
        resultText += tr("  • %1\n    路径: %2\n").arg(node->fileName).arg(node->filePath);
        displayedDLLs.append(node->fileName);
        count++;
    }
    if (uniqueDLLs.size() > 10) {
        resultText += tr("\n  ... 还有 %1 个\n").arg(uniqueDLLs.size() - 10);
    }
    resultText += tr("\n您现在可以点击【一键补齐】按钮来收集这些DLL。");
    
    QMessageBox::information(this, tr("高亮完成"), resultText);
    statusBar()->showMessage(tr("已高亮显示 %1 个缺失的DLL").arg(uniqueDLLs.size()));
}

void MainWindow::onExportMissingReport()
{
    if (m_scanResults.isEmpty()) {
        QMessageBox::warning(this, tr("无数据"), 
            tr("请先扫描文件夹或文件。\n\n"
               "建议：\n"
               "1. 点击【扫描文件夹】选择要扫描的文件夹\n"
               "2. 或点击【扫描文件】选择单个文件\n"
               "3. 等待扫描完成后即可导出报告"));
        return;
    }
    
    // 生成缺失报告
    ComparisonEngine::MissingReport report = 
        ComparisonEngine::generateMissingReport(m_scanResults);
    
    if (report.missingDLLs.isEmpty()) {
        QMessageBox::information(this, tr("无缺失DLL"), 
            tr("扫描结果中没有缺失的DLL。\n\n"
               "所有依赖的DLL都已找到，无需补齐。"));
        return;
    }
    
    // 选择保存位置
    QString filePath = QFileDialog::getSaveFileName(this, tr("导出缺失报告"),
                                                    "missing_report.json", 
                                                    tr("JSON 文件 (*.json)"));
    if (filePath.isEmpty()) {
        return;
    }
    
    // 保存缺失报告
    if (ComparisonEngine::saveMissingReport(report, filePath)) {
        statusBar()->showMessage(tr("缺失报告已导出: %1").arg(filePath));
        QMessageBox::information(this, tr("导出成功"), 
            tr("缺失报告已导出！\n\n"
               "找到 %1 个缺失的DLL。\n\n"
               "后续步骤：\n"
               "1. 将此报告文件复制到开发机\n"
               "2. 在开发机上使用【导入差异报告】功能\n"
               "3. 使用【一键补齐】功能收集缺失的DLL").arg(report.missingDLLs.size()));
    } else {
        QMessageBox::critical(this, tr("导出失败"), 
            tr("无法写入缺失报告文件。\n\n"
               "可能的原因：\n"
               "1. 没有写入权限\n"
               "2. 磁盘空间不足\n"
               "3. 文件路径无效\n\n"
               "建议：\n"
               "1. 以管理员身份运行本程序\n"
               "2. 选择其他保存位置\n"
               "3. 检查磁盘空间是否充足"));
    }
}

void MainWindow::onExportReport()
{
    if (m_scanResults.isEmpty()) {
        QMessageBox::warning(this, tr("无数据"), tr("请先扫描文件夹或文件。"));
        return;
    }
    
    QString filePath = QFileDialog::getSaveFileName(this, tr("导出报告"),
                                                    QString(), tr("文本文件 (*.txt);;HTML 文件 (*.html);;CSV 文件 (*.csv)"));
    if (filePath.isEmpty()) {
        return;
    }
    
    // Determine format based on extension
    ReportGenerator::ReportFormat format = ReportGenerator::PlainText;
    if (filePath.endsWith(".html", Qt::CaseInsensitive)) {
        format = ReportGenerator::HTML;
    } else if (filePath.endsWith(".csv", Qt::CaseInsensitive)) {
        format = ReportGenerator::CSV;
    }
    
    if (ReportGenerator::writeMissingReport(m_scanResults, format, filePath)) {
        statusBar()->showMessage(tr("报告已导出: %1").arg(filePath));
    } else {
        QMessageBox::critical(this, tr("导出失败"), tr("无法写入报告文件。"));
    }
}

void MainWindow::onTreeItemClicked(QTreeWidgetItem* item, int column)
{
    Q_UNUSED(column);
    if (item) {
        showDLLDetails(item);
    }
}

void MainWindow::onScanProgress(int current, int total, const QString& file)
{
    if (m_isDestroying) {
        return;
    }
    
    m_progressBar->setMaximum(total);
    m_progressBar->setValue(current);
    
    QString statusText = tr("正在扫描: %1 (%2/%3)").arg(file).arg(current).arg(total);
    
    if (total > 0 && current > 0 && m_scanStartTime > 0) {
        qint64 elapsedTime = QDateTime::currentMSecsSinceEpoch() - m_scanStartTime;
        qint64 avgTimePerFile = elapsedTime / current;
        qint64 remainingFiles = total - current;
        qint64 remainingTimeMs = remainingFiles * avgTimePerFile;
        
        if (remainingTimeMs > 0) {
            int remainingSeconds = static_cast<int>(remainingTimeMs / 1000);
            
            if (remainingSeconds < 60) {
                statusText += tr(" - 剩余时间: %1 秒").arg(remainingSeconds);
            } else if (remainingSeconds < 3600) {
                int minutes = remainingSeconds / 60;
                int seconds = remainingSeconds % 60;
                statusText += tr(" - 剩余时间: %1 分 %2 秒").arg(minutes).arg(seconds);
            } else {
                int hours = remainingSeconds / 3600;
                int minutes = (remainingSeconds % 3600) / 60;
                statusText += tr(" - 剩余时间: %1 小时 %2 分").arg(hours).arg(minutes);
            }
        }
    }
    
    statusBar()->showMessage(statusText);
}

void MainWindow::populateTree(DependencyScanner::DependencyNode* root)
{
    if (!root) return;
    
    QTreeWidgetItem* item = createTreeItem(root);
    m_treeWidget->addTopLevelItem(item);
    
    // 保存item到node的映射
    m_itemNodeMap[item] = root;
    
    // 自动展开包含缺失项的节点
    expandMissingNodes(item);
}

void MainWindow::showDLLDetails(QTreeWidgetItem* item)
{
    if (!item) {
        m_detailPanel->setHtml(tr("<h3>未选择任何DLL</h3>"));
        return;
    }

    auto it = m_itemNodeMap.find(item);
    if (it == m_itemNodeMap.end()) {
        m_detailPanel->setHtml(tr("<h3>未找到DLL信息</h3>"));
        return;
    }

    DependencyScanner::DependencyNode* node = it.value();
    if (!node) {
        m_detailPanel->setHtml(tr("<h3>DLL信息不可用</h3>"));
        return;
    }

    QString details = tr("<h3>DLL 详细信息</h3>");

    details += tr("<h4>基本信息</h4>");
    details += tr("<table border='0' cellpadding='5' cellspacing='0'>");
    details += tr("<tr><td width='120'><b>文件名:</b></td><td>%1</td></tr>").arg(node->fileName);
    details += tr("<tr><td><b>完整路径:</b></td><td>%1</td></tr>").arg(node->filePath);
    
    QFileInfo fileInfo(node->filePath);
    if (fileInfo.exists()) {
        details += tr("<tr><td><b>文件大小:</b></td><td>%1</td></tr>")
            .arg(formatFileSize(fileInfo.size()));
        details += tr("<tr><td><b>修改时间:</b></td><td>%1</td></tr>")
            .arg(fileInfo.lastModified().toString("yyyy-MM-dd hh:mm:ss"));
    }
    
    QString archStr = PEParser::architectureToString(node->arch);
    details += tr("<tr><td><b>架构:</b></td><td>%1</td></tr>").arg(archStr);
    details += tr("</table>");

    details += tr("<h4>版本信息</h4>");
    details += tr("<table border='0' cellpadding='5' cellspacing='0'>");
    details += tr("<tr><td width='120'><b>文件版本:</b></td><td>%1</td></tr>")
        .arg(node->fileVersion.isEmpty() ? tr("未知") : node->fileVersion);
    details += tr("<tr><td><b>产品版本:</b></td><td>%1</td></tr>")
        .arg(node->productVersion.isEmpty() ? tr("未知") : node->productVersion);
    details += tr("</table>");

    details += tr("<h4>状态信息</h4>");
    details += tr("<table border='0' cellpadding='5' cellspacing='0'>");
    if (node->exists) {
        details += tr("<tr><td width='120'><b>状态:</b></td><td><font color='green'>正常</font></td></tr>");
    } else {
        details += tr("<tr><td width='120'><b>状态:</b></td><td><font color='red'>缺失</font></td></tr>");
        details += tr("<tr><td></td><td><i>该DLL文件不存在于系统中，可能导致程序无法正常运行。</i></td></tr>");
    }
    
    if (node->archMismatch) {
        details += tr("<tr><td><b>架构:</b></td><td><font color='orange'>不匹配</font></td></tr>");
        details += tr("<tr><td></td><td><i>DLL架构与主程序不匹配，可能导致加载失败。</i></td></tr>");
    } else {
        details += tr("<tr><td><b>架构:</b></td><td>匹配</td></tr>");
    }
    details += tr("</table>");

    details += tr("<h4>依赖关系</h4>");
    details += tr("<table border='0' cellpadding='5' cellspacing='0'>");
    details += tr("<tr><td width='120'><b>依赖DLL数:</b></td><td>%1</td></tr>")
        .arg(node->children.size());
    
    int missingChildren = 0;
    for (const auto& child : node->children) {
        if (!child->exists) {
            missingChildren++;
        }
    }
    details += tr("<tr><td><b>缺失依赖:</b></td><td>%1</td></tr>")
        .arg(missingChildren);
    
    details += tr("<tr><td><b>依赖层级:</b></td><td>%1</td></tr>")
        .arg(node->depth);
    
    if (node->parent) {
        details += tr("<tr><td><b>被依赖:</b></td><td>是 (被 %1 个文件依赖)</td></tr>")
            .arg(1);
    } else {
        details += tr("<tr><td><b>被依赖:</b></td><td>否 (根节点)</td></tr>");
    }
    details += tr("</table>");

    if (missingChildren > 0) {
        details += tr("<h4>缺失的依赖DLL</h4>");
        details += tr("<ul>");
        for (const auto& child : node->children) {
            if (!child->exists) {
                details += tr("<li><font color='red'>%1</font> (%2)</li>")
                    .arg(child->fileName)
                    .arg(child->filePath);
            }
        }
        details += tr("</ul>");
    }

    m_detailPanel->setHtml(details);
}

QTreeWidgetItem* MainWindow::createTreeItem(DependencyScanner::DependencyNode* node)
{
    QTreeWidgetItem* item = new QTreeWidgetItem();
    item->setText(0, node->fileName);
    item->setText(1, node->filePath);
    item->setText(2, PEParser::architectureToString(node->arch));
    item->setText(3, node->fileVersion);
    item->setText(4, node->exists ? tr("正常") : tr("缺失"));
    item->setData(1, Qt::UserRole, node->filePath);
    
    // 保存item到node的映射
    m_itemNodeMap[item] = node;
    
    // Set color based on status
    if (!node->exists) {
        item->setForeground(4, Qt::red);
    } else if (node->archMismatch) {
        item->setForeground(4, QColor(255, 165, 0)); // Orange
    }
    
    // Add children
    for (const auto& child : node->children) {
        item->addChild(createTreeItem(child.get()));
    }
    
    return item;
}

void MainWindow::expandMissingNodes(QTreeWidgetItem* item)
{
    if (!item) return;
    
    // 检查当前节点或其子节点是否包含缺失项
    if (hasMissingDependencies(item)) {
        // 展开当前节点
        item->setExpanded(true);
        
        // 递归展开所有子节点
        for (int i = 0; i < item->childCount(); ++i) {
            expandMissingNodes(item->child(i));
        }
    } else {
        // 如果没有缺失项，保持折叠状态（默认行为）
        item->setExpanded(false);
    }
}

bool MainWindow::hasMissingDependencies(QTreeWidgetItem* item)
{
    if (!item) return false;
    
    // 检查当前节点是否为缺失状态
    QString status = item->text(4);
    if (status == tr("缺失")) {
        return true;
    }
    
    // 递归检查所有子节点
    for (int i = 0; i < item->childCount(); ++i) {
        if (hasMissingDependencies(item->child(i))) {
            return true;
        }
    }
    
    return false;
}

void MainWindow::highlightMissingDLLs(const QStringList& missingDLLs)
{
    // 清除之前的映射
    m_itemNodeMap.clear();
    
    // 遍历所有顶层项
    for (int i = 0; i < m_treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = m_treeWidget->topLevelItem(i);
        if (i < m_scanResults.size()) {
            highlightTreeItem(item, missingDLLs, m_scanResults[i]);
        }
    }
}

void MainWindow::highlightTreeItem(QTreeWidgetItem* item, const QStringList& missingDLLs, 
                                   DependencyScanner::DependencyNode* node)
{
    if (!item || !node) return;
    
    // 保存item到node的映射
    m_itemNodeMap[item] = node;
    
    // 检查当前节点是否在缺失列表中
    if (missingDLLs.contains(node->fileName, Qt::CaseInsensitive)) {
        // 高亮显示：使用黄色背景
        for (int col = 0; col < item->columnCount(); ++col) {
            item->setBackground(col, QColor(255, 255, 0, 100));  // 半透明黄色
        }
        // 设置粗体
        QFont font = item->font(0);
        font.setBold(true);
        for (int col = 0; col < item->columnCount(); ++col) {
            item->setFont(col, font);
        }
        
        // 展开到此节点的路径
        QTreeWidgetItem* parent = item->parent();
        while (parent) {
            parent->setExpanded(true);
            parent = parent->parent();
        }
        item->setExpanded(true);
    }
    
    // 递归处理子节点
    for (int i = 0; i < item->childCount(); ++i) {
        if (i < static_cast<int>(node->children.size())) {
            highlightTreeItem(item->child(i), missingDLLs, node->children[i].get());
        }
    }
}

QList<DependencyScanner::DependencyNode*> MainWindow::getHighlightedNodes()
{
    return m_highlightedNodes;
}

void MainWindow::clearAllData()
{
    m_treeWidget->clear();
    m_detailPanel->clear();
    m_itemNodeMap.clear();
    m_highlightedNodes.clear();
    qDeleteAll(m_scanResults);
    m_scanResults.clear();
    statusBar()->showMessage(tr("数据已清空"));
}

void MainWindow::onClearAll()
{
    int ret = QMessageBox::question(this, tr("确认清空"), 
        tr("确定要清空所有扫描结果吗？"),
        QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        clearAllData();
    }
}

void MainWindow::onAutoCollectDLLs()
{
    // 步骤1: 检查是否有高亮节点
    if (m_highlightedNodes.isEmpty()) {
        QMessageBox::warning(this, tr("请先导入差异报告"), 
            tr("请先使用【导入差异报告】功能加载目标机的缺失报告，\n"
               "系统会自动高亮显示缺失的DLL，然后您可以使用此功能进行一键补齐。\n\n"
               "操作步骤：\n"
               "1. 先在目标机上使用【导出缺失报告】功能\n"
               "2. 将报告文件复制到开发机\n"
               "3. 在开发机上使用【导入差异报告】功能\n"
               "4. 然后使用【一键补齐】功能"));
        return;
    }
    
    // 步骤1.5: 去重 - 每个DLL只保留一个节点
    QMap<QString, DependencyScanner::DependencyNode*> uniqueNodes;
    for (auto* node : m_highlightedNodes) {
        if (!uniqueNodes.contains(node->fileName)) {
            uniqueNodes.insert(node->fileName, node);
        }
    }
    QList<DependencyScanner::DependencyNode*> nodesToCollect = uniqueNodes.values();
    
    // 步骤2: 显示高亮DLL列表预览
    QString previewText = tr("找到 %1 个高亮的 DLL 需要收集:\n\n").arg(nodesToCollect.size());
    int count = 0;
    qint64 totalSize = 0;
    for (const auto* node : nodesToCollect) {
        if (count < 10) {
            QFileInfo fileInfo(node->filePath);
            qint64 fileSize = fileInfo.size();
            totalSize += fileSize;
            previewText += tr("  • %1\n    路径: %2\n    大小: %3 KB\n")
                .arg(node->fileName)
                .arg(node->filePath)
                .arg(fileSize / 1024.0, 0, 'f', 2);
            count++;
        } else {
            QFileInfo fileInfo(node->filePath);
            totalSize += fileInfo.size();
        }
    }
    if (nodesToCollect.size() > 10) {
        previewText += tr("\n  ... 还有 %1 个\n").arg(nodesToCollect.size() - 10);
    }
    previewText += tr("\n总大小: %1 MB\n").arg(totalSize / 1024.0 / 1024.0, 0, 'f', 2);
    previewText += tr("\n是否继续?");
    
    int ret = QMessageBox::question(this, tr("确认收集"), previewText,
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }
    
    // 步骤3: 选择目标文件夹
    QString targetDir = QFileDialog::getExistingDirectory(
        this, 
        tr("选择收集 DLL 的目标文件夹")
    );
    
    if (targetDir.isEmpty()) {
        return;
    }
    
    // 验证目标目录
    InputValidator::ValidationResult validation = InputValidator::validateTargetDirectory(targetDir);
    if (validation != InputValidator::Valid) {
        LOG_WARNING("MainWindow", QString("目标目录验证失败: %1 - %2").arg(targetDir).arg(InputValidator::getErrorMessage(validation)));
        QMessageBox::warning(this, tr("目录验证失败"), 
            tr("无法使用此目录作为目标：\n%1\n\n原因：%2")
            .arg(targetDir)
            .arg(InputValidator::getErrorMessage(validation)));
        return;
    }
    
    // 步骤4: 选择冲突处理策略
    QMessageBox conflictBox;
    conflictBox.setWindowTitle(tr("冲突解决"));
    conflictBox.setText(tr("如何处理已存在的文件?\n\n"
                           "说明：\n"
                           "- 全部覆盖：将覆盖目标文件夹中已存在的同名文件\n"
                           "- 全部跳过：保留已存在的文件，不进行复制"));
    QPushButton* overwriteBtn = conflictBox.addButton(tr("全部覆盖"), QMessageBox::AcceptRole);
    QPushButton* skipBtn = conflictBox.addButton(tr("全部跳过"), QMessageBox::RejectRole);
    conflictBox.addButton(QMessageBox::Cancel);
    conflictBox.exec();
    
    DLLCollector::ConflictResolution conflictMode;
    if (conflictBox.clickedButton() == overwriteBtn) {
        conflictMode = DLLCollector::Overwrite;
    } else if (conflictBox.clickedButton() == skipBtn) {
        conflictMode = DLLCollector::Skip;
    } else {
        return;  // 用户取消
    }
    
    // 步骤5: 执行复制并显示进度
    QProgressDialog progressDialog(tr("正在收集 DLL..."), tr("取消"), 
                                   0, nodesToCollect.size(), this);
    progressDialog.setWindowModality(Qt::WindowModal);
    progressDialog.setMinimumDuration(0);
    
    DLLCollector* collector = new DLLCollector(this);
    
    connect(collector, &DLLCollector::copyProgress, 
            [&progressDialog](int current, int total, const QString& fileName) {
        progressDialog.setValue(current);
        progressDialog.setLabelText(tr("正在复制: %1 (%2/%3)").arg(fileName).arg(current).arg(total));
    });
    
    DLLCollector::CollectionResult result = collector->collectDLLsFromNodes(
        nodesToCollect, targetDir, conflictMode
    );
    
    progressDialog.setValue(result.totalFiles);
    
    // 步骤6: 显示结果摘要
    QString resultText = tr("收集完成!\n\n");
    resultText += tr("总文件数: %1\n").arg(result.totalFiles);
    resultText += tr("成功复制: %2\n").arg(result.successCount);
    resultText += tr("失败: %3\n").arg(result.failedCount);
    
    if (result.failedCount > 0) {
        resultText += tr("\n失败的文件:\n");
        for (auto it = result.failedFiles.begin(); it != result.failedFiles.end(); ++it) {
            resultText += tr("  • %1: %2\n").arg(it.key()).arg(it.value());
        }
        resultText += tr("\n可能的原因：\n"
            "1. 源文件不存在\n"
            "2. 没有读取权限\n"
            "3. 磁盘空间不足\n"
            "4. 目标路径无效");
    }
    
    QMessageBox resultBox;
    resultBox.setWindowTitle(tr("收集结果"));
    resultBox.setText(resultText);
    resultBox.setIcon(result.failedCount == 0 ? QMessageBox::Information : QMessageBox::Warning);
    
    QPushButton* openFolderBtn = resultBox.addButton(tr("打开目标文件夹"), QMessageBox::ActionRole);
    resultBox.addButton(QMessageBox::Ok);

    resultBox.exec();

    if (resultBox.clickedButton() == openFolderBtn) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(targetDir));
    }

    delete collector;
}

void MainWindow::onScanFinished(QList<DependencyScanner::DependencyNode*> results)
{
    if (m_isDestroying) {
        return;
    }
    
    m_isScanning = false;
    m_progressBar->setVisible(false);

    QAction* cancelAction = m_toolBar->findChild<QAction*>("cancelAction");
    if (cancelAction) {
        cancelAction->setEnabled(false);
    }

    m_scanResults = results;
    m_highlightedNodes.clear();

    for (auto* root : m_scanResults) {
        populateTree(root);
    }

    statusBar()->showMessage(tr("扫描完成。找到 %1 个文件。").arg(m_scanResults.size()));
    
    m_scanThread = nullptr;
    m_scanWorker = nullptr;
}

QString MainWindow::formatErrorWithSuggestion(const QString& errorMessage)
{
    QString fullMessage = errorMessage;
    QString suggestion;
    
    if (errorMessage.contains("File does not exist", Qt::CaseInsensitive) || 
        errorMessage.contains("文件不存在", Qt::CaseInsensitive)) {
        suggestion = tr("\n\n建议：\n"
            "1. 检查文件路径是否正确\n"
            "2. 确认文件是否已被移动或删除\n"
            "3. 检查是否有足够的文件访问权限");
    } else if (errorMessage.contains("Invalid PE file", Qt::CaseInsensitive) ||
               errorMessage.contains("无效的PE文件", Qt::CaseInsensitive)) {
        suggestion = tr("\n\n建议：\n"
            "1. 确认文件是否为有效的Windows可执行文件（.exe或.dll）\n"
            "2. 文件可能已损坏，尝试重新获取该文件\n"
            "3. 如果是跨平台编译的文件，请确保是Windows版本");
    } else if (errorMessage.contains("permission", Qt::CaseInsensitive) ||
               errorMessage.contains("权限", Qt::CaseInsensitive) ||
               errorMessage.contains("access denied", Qt::CaseInsensitive)) {
        suggestion = tr("\n\n建议：\n"
            "1. 以管理员身份运行本程序\n"
            "2. 检查文件和文件夹的安全权限设置\n"
            "3. 确保文件未被其他程序锁定");
    } else if (errorMessage.contains("disk", Qt::CaseInsensitive) ||
               errorMessage.contains("磁盘", Qt::CaseInsensitive) ||
               errorMessage.contains("space", Qt::CaseInsensitive)) {
        suggestion = tr("\n\n建议：\n"
            "1. 检查磁盘空间是否充足\n"
            "2. 清理临时文件释放磁盘空间\n"
            "3. 尝试选择其他磁盘位置进行扫描");
    } else if (errorMessage.contains("cancelled", Qt::CaseInsensitive) ||
               errorMessage.contains("已取消", Qt::CaseInsensitive)) {
        suggestion = tr("\n\n提示：\n"
            "扫描已被用户取消。您可以重新开始扫描。");
    } else if (errorMessage.contains("无法扫描文件", Qt::CaseInsensitive)) {
        suggestion = tr("\n\n建议：\n"
            "1. 确认文件格式正确（应为.exe或.dll）\n"
            "2. 检查文件是否完整且未损坏\n"
            "3. 尝试以管理员权限运行本程序");
    } else {
        suggestion = tr("\n\n建议：\n"
            "1. 以管理员身份运行本程序\n"
            "2. 检查文件路径和权限\n"
            "3. 确认文件未损坏且格式正确\n"
            "4. 如问题持续，请联系技术支持");
    }
    
    fullMessage += suggestion;
    return fullMessage;
}

QString MainWindow::formatFileSize(qint64 bytes)
{
    const qint64 KB = 1024;
    const qint64 MB = 1024 * KB;
    const qint64 GB = 1024 * MB;
    
    if (bytes < KB) {
        return tr("%1 B").arg(bytes);
    } else if (bytes < MB) {
        return tr("%1 KB").arg(QString::number(bytes / (double)KB, 'f', 2));
    } else if (bytes < GB) {
        return tr("%1 MB").arg(QString::number(bytes / (double)MB, 'f', 2));
    } else {
        return tr("%1 GB").arg(QString::number(bytes / (double)GB, 'f', 2));
    }
}

void MainWindow::onScanError(const QString& errorMessage)
{
    if (m_isDestroying) {
        return;
    }
    
    m_isScanning = false;
    m_progressBar->setVisible(false);

    QAction* cancelAction = m_toolBar->findChild<QAction*>("cancelAction");
    if (cancelAction) {
        cancelAction->setEnabled(false);
    }

    QString detailedError = formatErrorWithSuggestion(errorMessage);
    QMessageBox::critical(this, tr("扫描错误"), detailedError);
    statusBar()->showMessage(tr("扫描失败"));
    
    m_scanThread = nullptr;
    m_scanWorker = nullptr;
}

void MainWindow::onCancelScan()
{
    if (m_isScanning && m_scanWorker) {
        m_scanWorker->cancel();
        statusBar()->showMessage(tr("正在取消扫描..."));
        
        QAction* cancelAction = m_toolBar->findChild<QAction*>("cancelAction");
        if (cancelAction) {
            cancelAction->setEnabled(false);
        }
    }
}

