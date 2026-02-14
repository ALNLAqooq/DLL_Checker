# PE Parser Unit Tests

This directory contains unit tests for the PEParser module (Task 2.2).

## Files

- `test_peparser.cpp` - QtTest-based unit tests for PEParser
- `tests.pro` - Qt project file for building the tests

## Building the Tests

### Using Qt Creator
1. Open `tests.pro` in Qt Creator
2. Configure the kit (Desktop Qt with MSVC)
3. Build the project
4. Run the executable `test_peparser.exe`

### Using qmake from Command Line
```bash
cd tests
qmake tests.pro
nmake  # or mingw32-make for MinGW
.\release\test_peparser.exe
```

## Test Coverage

The tests cover the following scenarios:

1. **x86 Architecture Detection** - Creates a temporary PE file with x86 machine type and verifies correct identification
2. **x64 Architecture Detection** - Creates a temporary PE file with x64 machine type and verifies correct identification
3. **Non-existent File** - Tests error handling for non-existent files
4. **Empty File** - Tests handling of empty files
5. **Invalid DOS Header** - Tests detection of corrupted DOS headers
6. **Invalid PE Header** - Tests detection of invalid PE signatures
7. **Corrupted PE Header** - Tests handling of truncated file headers

## Requirements Validated

These tests validate **Requirement 5.1**: "WHEN 解析PE文件 THEN THE PE_Parser SHALL 识别文件架构（x86、x64或Any CPU）"

## Implementation Notes

- The tests use temporary files with simulated PE headers to avoid dependency on system DLLs
- The `createTempPEFile()` helper function generates minimal valid PE headers for testing
- All tests follow the Arrange-Act-Assert pattern
- The tests verify both `getArchitecture()` and `parsePEFile()` methods

## Integration with Main Project

To integrate these tests into the main build system, you can:

1. Add a `test` target to the main `DLLChecker.pro` file
2. Use Qt's `add_test` in CMake (if using CMake)
3. Run tests as part of CI/CD pipeline

## Future Enhancements

1. Add property-based testing for more comprehensive coverage
2. Test `getImportedDLLs()` with actual PE files
3. Test `getVersionInfo()` with version resources
4. Add performance benchmarks for large PE files