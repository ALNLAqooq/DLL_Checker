#include "peparser.h"
#include "comparisonengine.h"
#include <QtTest>
#include <QTemporaryFile>
#include <QFile>
#include <QDebug>
#include <memory>

class TestPEParser : public QObject
{
    Q_OBJECT

private slots:
    void testX86Architecture();
    void testX64Architecture();
    void testNonExistentFile();
    void testEmptyFile();
    void testInvalidDOSHeader();
    void testInvalidPEHeader();
    void testCorruptedPEHeader();
    void testMissingReportDedupAndRoundTrip();
    void testFindMissingDLLsInTree();

private:
    bool createTempPEFile(QTemporaryFile& file, quint16 machine);
    DependencyScanner::DependencyNode* createNode(const QString& fileName, const QString& filePath, bool exists);
};

bool TestPEParser::createTempPEFile(QTemporaryFile& file, quint16 machine)
{
    if (!file.open()) {
        return false;
    }

    // Write DOS header
    IMAGE_DOS_HEADER dosHeader = {0};
    dosHeader.e_magic = IMAGE_DOS_SIGNATURE;  // "MZ"
    dosHeader.e_lfanew = sizeof(IMAGE_DOS_HEADER);  // PE header starts right after DOS header

    if (file.write((const char*)&dosHeader, sizeof(dosHeader)) != sizeof(dosHeader)) {
        return false;
    }

    // Write PE signature
    DWORD peSignature = IMAGE_NT_SIGNATURE;
    if (file.write((const char*)&peSignature, sizeof(peSignature)) != sizeof(peSignature)) {
        return false;
    }

    // Write file header
    IMAGE_FILE_HEADER fileHeader = {0};
    fileHeader.Machine = machine;
    fileHeader.NumberOfSections = 1;
    fileHeader.SizeOfOptionalHeader = 0;
    fileHeader.Characteristics = IMAGE_FILE_EXECUTABLE_IMAGE | IMAGE_FILE_32BIT_MACHINE;

    if (file.write((const char*)&fileHeader, sizeof(fileHeader)) != sizeof(fileHeader)) {
        return false;
    }

    // Write optional header (minimal)
    // For simplicity, we write a small optional header
    // Actual size depends on architecture, but we just need enough for the parser
    // The parser only reads the file header, so optional header is not needed
    // However, we'll write some bytes to make the file look valid
    char optionalHeader[128] = {0};
    if (file.write(optionalHeader, sizeof(optionalHeader)) != sizeof(optionalHeader)) {
        return false;
    }

    file.flush();
    return true;
}

DependencyScanner::DependencyNode* TestPEParser::createNode(const QString& fileName,
                                                            const QString& filePath,
                                                            bool exists)
{
    auto* node = new DependencyScanner::DependencyNode();
    node->fileName = fileName;
    node->filePath = filePath;
    node->exists = exists;
    return node;
}

void TestPEParser::testX86Architecture()
{
    QTemporaryFile tempFile;
    if (!createTempPEFile(tempFile, IMAGE_FILE_MACHINE_I386)) {
        QFAIL("Failed to create temporary x86 PE file");
    }

    PEParser::Architecture arch = PEParser::getArchitecture(tempFile.fileName());
    QCOMPARE(arch, PEParser::x86);
    
    // Also test parsePEFile
    PEParser::PEInfo info = PEParser::parsePEFile(tempFile.fileName());
    QVERIFY(info.isValid);
    QCOMPARE(info.arch, PEParser::x86);
    QVERIFY(info.errorMessage.isEmpty());
}

void TestPEParser::testX64Architecture()
{
    QTemporaryFile tempFile;
    if (!createTempPEFile(tempFile, IMAGE_FILE_MACHINE_AMD64)) {
        QFAIL("Failed to create temporary x64 PE file");
    }

    PEParser::Architecture arch = PEParser::getArchitecture(tempFile.fileName());
    QCOMPARE(arch, PEParser::x64);
    
    // Also test parsePEFile
    PEParser::PEInfo info = PEParser::parsePEFile(tempFile.fileName());
    QVERIFY(info.isValid);
    QCOMPARE(info.arch, PEParser::x64);
    QVERIFY(info.errorMessage.isEmpty());
}

void TestPEParser::testNonExistentFile()
{
    QString nonExistentPath = "C:\\this_file_does_not_exist_12345.dll";
    PEParser::Architecture arch = PEParser::getArchitecture(nonExistentPath);
    QCOMPARE(arch, PEParser::Unknown);
    
    PEParser::PEInfo info = PEParser::parsePEFile(nonExistentPath);
    QVERIFY(!info.isValid);
    QVERIFY(!info.errorMessage.isEmpty());
    QCOMPARE(info.arch, PEParser::Unknown);
}

void TestPEParser::testEmptyFile()
{
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QFAIL("Failed to create temporary file");
    }
    // File is empty
    
    PEParser::Architecture arch = PEParser::getArchitecture(tempFile.fileName());
    QCOMPARE(arch, PEParser::Unknown);
    
    PEParser::PEInfo info = PEParser::parsePEFile(tempFile.fileName());
    QVERIFY(!info.isValid);
    QVERIFY(!info.errorMessage.isEmpty());
    QCOMPARE(info.arch, PEParser::Unknown);
}

void TestPEParser::testInvalidDOSHeader()
{
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QFAIL("Failed to create temporary file");
    }
    
    // Write invalid DOS signature
    IMAGE_DOS_HEADER dosHeader = {0};
    dosHeader.e_magic = 0x1234;  // Invalid
    dosHeader.e_lfanew = sizeof(IMAGE_DOS_HEADER);
    
    tempFile.write((const char*)&dosHeader, sizeof(dosHeader));
    tempFile.flush();
    
    PEParser::Architecture arch = PEParser::getArchitecture(tempFile.fileName());
    QCOMPARE(arch, PEParser::Unknown);
    
    PEParser::PEInfo info = PEParser::parsePEFile(tempFile.fileName());
    QVERIFY(!info.isValid);
    QVERIFY(!info.errorMessage.isEmpty());
    QCOMPARE(info.arch, PEParser::Unknown);
}

void TestPEParser::testInvalidPEHeader()
{
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QFAIL("Failed to create temporary file");
    }
    
    // Write valid DOS header
    IMAGE_DOS_HEADER dosHeader = {0};
    dosHeader.e_magic = IMAGE_DOS_SIGNATURE;
    dosHeader.e_lfanew = sizeof(IMAGE_DOS_HEADER);
    tempFile.write((const char*)&dosHeader, sizeof(dosHeader));
    
    // Write invalid PE signature
    DWORD peSignature = 0x12345678;
    tempFile.write((const char*)&peSignature, sizeof(peSignature));
    tempFile.flush();
    
    PEParser::Architecture arch = PEParser::getArchitecture(tempFile.fileName());
    QCOMPARE(arch, PEParser::Unknown);
    
    PEParser::PEInfo info = PEParser::parsePEFile(tempFile.fileName());
    QVERIFY(!info.isValid);
    QVERIFY(!info.errorMessage.isEmpty());
    QCOMPARE(info.arch, PEParser::Unknown);
}

void TestPEParser::testCorruptedPEHeader()
{
    QTemporaryFile tempFile;
    if (!tempFile.open()) {
        QFAIL("Failed to create temporary file");
    }
    
    // Write valid DOS header
    IMAGE_DOS_HEADER dosHeader = {0};
    dosHeader.e_magic = IMAGE_DOS_SIGNATURE;
    dosHeader.e_lfanew = sizeof(IMAGE_DOS_HEADER);
    tempFile.write((const char*)&dosHeader, sizeof(dosHeader));
    
    // Write valid PE signature
    DWORD peSignature = IMAGE_NT_SIGNATURE;
    tempFile.write((const char*)&peSignature, sizeof(peSignature));
    
    // Write truncated file header (less than full size)
    IMAGE_FILE_HEADER fileHeader = {0};
    fileHeader.Machine = IMAGE_FILE_MACHINE_I386;
    // Write only half of the header
    tempFile.write((const char*)&fileHeader, sizeof(fileHeader) / 2);
    tempFile.flush();
    
    PEParser::Architecture arch = PEParser::getArchitecture(tempFile.fileName());
    QCOMPARE(arch, PEParser::Unknown);
    
    PEParser::PEInfo info = PEParser::parsePEFile(tempFile.fileName());
    QVERIFY(!info.isValid);
    QVERIFY(!info.errorMessage.isEmpty());
    QCOMPARE(info.arch, PEParser::Unknown);
}

void TestPEParser::testMissingReportDedupAndRoundTrip()
{
    auto* root = createNode("app.exe", "C:/app/app.exe", true);
    auto* missingA = createNode("vcruntime140.dll", "vcruntime140.dll", false);
    auto* missingB = createNode("vcruntime140.dll", "vcruntime140.dll", false);

    missingA->parent = root;
    missingB->parent = root;
    root->children.emplace_back(missingA);
    root->children.emplace_back(missingB);

    QList<DependencyScanner::DependencyNode*> roots;
    roots.append(root);

    const ComparisonEngine::MissingReport report = ComparisonEngine::generateMissingReport(roots);
    QCOMPARE(report.missingDLLs.size(), 1);
    QCOMPARE(report.missingDLLs.first().toLower(), QString("vcruntime140.dll"));

    QTemporaryFile file;
    QVERIFY(file.open());
    const QString path = file.fileName();
    file.close();

    QVERIFY(ComparisonEngine::saveMissingReport(report, path));
    const ComparisonEngine::MissingReport loaded = ComparisonEngine::loadMissingReport(path);
    QCOMPARE(loaded.missingDLLs.size(), 1);
    QCOMPARE(loaded.missingDLLs.first().toLower(), QString("vcruntime140.dll"));

    delete root;
}

void TestPEParser::testFindMissingDLLsInTree()
{
    auto* root = createNode("app.exe", "C:/app/app.exe", true);
    auto* present = createNode("Qt5Core.dll", "C:/app/Qt5Core.dll", true);
    auto* missing = createNode("msvcp140.dll", "msvcp140.dll", false);
    present->parent = root;
    missing->parent = root;

    root->children.emplace_back(present);
    root->children.emplace_back(missing);

    ComparisonEngine::MissingReport report;
    report.missingDLLs << "msvcp140.dll";

    QList<DependencyScanner::DependencyNode*> roots;
    roots.append(root);

    const QList<DependencyScanner::DependencyNode*> found =
        ComparisonEngine::findMissingDLLsInTree(roots, report);

    QCOMPARE(found.size(), 1);
    QCOMPARE(found.first()->fileName.toLower(), QString("msvcp140.dll"));
    QVERIFY(!found.first()->exists);

    delete root;
}

QTEST_APPLESS_MAIN(TestPEParser)

#include "test_peparser.moc"
