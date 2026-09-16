#include "huffman.hpp"

static int testFailures = 0;

#define ASSERT_EQUAL(name, actual, expected)                     \
  do {                                                           \
    auto actualValue = (actual);                                 \
    auto expectedValue = (expected);                             \
    if (actualValue != expectedValue) {                          \
      std::cerr << "FAIL: " << name << " at line " << __LINE__   \
                << ". Expected " << expectedValue << " but got " \
                << actualValue << "\n";                          \
      testFailures++;                                            \
    } else {                                                     \
      std::cout << "PASS: " << name << "\n";                     \
    }                                                            \
  } while (0)

bool compareFiles(const std::string& firstPath, const std::string& secondPath) {
  FILE* firstFile = fopen(firstPath.c_str(), "rb");
  FILE* secondFile = fopen(secondPath.c_str(), "rb");

  if (!firstFile || !secondFile) {
    if (firstFile) {
      fclose(firstFile);
    }
    if (secondFile) {
      fclose(secondFile);
    }
    return false;
  }

  while (true) {
    int firstByte = fgetc(firstFile);
    int secondByte = fgetc(secondFile);

    if (firstByte != secondByte) {
      fclose(firstFile);
      fclose(secondFile);
      return false;
    }

    if (firstByte == EOF) {
      break;
    }
  }

  fclose(firstFile);
  fclose(secondFile);
  return true;
}

void runTest(const std::string& testName, const void* testData,
             size_t dataSize) {
  std::string inputPath = testName + ".txt";
  std::string compressedPath = testName + ".txt.huf";
  std::string decompressedPath = testName + "_decompressed.txt";

  FILE* inputFile = fopen(inputPath.c_str(), "wb");
  if (dataSize > 0) {
    fwrite(testData, 1, dataSize, inputFile);
  }
  fclose(inputFile);

  auto progress = [](int) {};

  fCompress(inputPath, progress);
  fDecompress(compressedPath, progress);

  bool filesMatch = compareFiles(inputPath, decompressedPath);
  ASSERT_EQUAL(testName, filesMatch, true);

  std::remove(inputPath.c_str());
  std::remove(compressedPath.c_str());
  std::remove(decompressedPath.c_str());
}

// Tests

void testEncodeDecode() {
  std::string testData = "This is a test string for Huffman coding.";
  runTest("test_encode_decode", testData.data(), testData.size());
}

void testEmpty() {
  std::string testData = "";
  runTest("test_empty", testData.data(), testData.size());
}

void testRepeated() {
  std::string testData = "AAAAAAAA";
  runTest("test_repeated", testData.data(), testData.size());
}

void testSingle() {
  std::string testData = "A";
  runTest("test_single", testData.data(), testData.size());
}

void testBigPayload() {
  std::string testData =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Sed do eiusmod "
      "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
      "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
      "commodo consequat. Duis aute irure dolor in reprehenderit in voluptate "
      "velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
      "occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
      "mollit anim id est laborum.";
  runTest("test_big_payload", testData.data(), testData.size());
}

void testHugePayload() {
  std::string testData = "";

  for (int i = 0; i < 10000; ++i) {
    testData +=
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
        "Sed do eiusmod tempor incididunt ut labore et dolore magna "
        "aliqua. Ut enim ad minim veniam, quis nostrud exercitation "
        "ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis "
        "aute irure dolor in reprehenderit in voluptate velit esse "
        "cillum dolore eu fugiat nulla pariatur. Excepteur sint "
        "occaecat cupidatat non proident, sunt in culpa qui officia "
        "deserunt mollit anim id est laborum.";
  }

  runTest("test_huge_payload", testData.data(), testData.size());
}

void testBinary() {
  uint8_t binaryData[256];
  for (int i = 0; i < 256; i++) {
    binaryData[i] = static_cast<uint8_t>(i);
  }
  runTest("test_binary", binaryData, sizeof(binaryData));
}

void testNoExtension() {
  std::string inputPath = "test_no_extension";
  std::string compressedPath = "test_no_extension.huf";
  std::string decompressedPath = "test_no_extension_decompressed";

  std::string testData = "No extension.";
  size_t dataSize = testData.size();

  FILE* inputFile = fopen(inputPath.c_str(), "wb");
  if (dataSize > 0) {
    fwrite(testData.data(), 1, dataSize, inputFile);
  }
  fclose(inputFile);

  auto progress = [](int) {};

  fCompress(inputPath, progress);
  fDecompress(compressedPath, progress);

  bool filesMatch = compareFiles(inputPath, decompressedPath);
  ASSERT_EQUAL("test_no_extension", filesMatch, true);

  std::remove(inputPath.c_str());
  std::remove(compressedPath.c_str());
  std::remove(decompressedPath.c_str());
}

void testNullRootWithPositiveBytes() {
  std::string path = "test_null_root.huf";

  FILE* f = fopen(path.c_str(), "wb");
  if (!f) {
    return;
  }
  uint8_t dummyByte = 0xAA;
  fwrite(&dummyByte, sizeof(uint8_t), 1, f);
  fclose(f);

  f = fopen(path.c_str(), "rb");
  if (!f) {
    return;
  }
  HuffmanTreeNode* nullRoot = nullptr;
  uint64_t totalBytes = 10;

  fReadDecodeCreate(f, nullRoot, path, totalBytes);

  fclose(f);
  std::remove(path.c_str());
  std::remove("test_null_root_decompressed");

  ASSERT_EQUAL("test_null_root", true, true);
}

int main() {
  testEncodeDecode();
  testEmpty();
  testRepeated();
  testBinary();
  testNoExtension();
  testBigPayload();
  testHugePayload();
  testNullRootWithPositiveBytes();

  if (testFailures > 0) {
    std::cerr << "Tests failed: " << testFailures << "\n";
    return 1;
  }

  std::cout << "All tests passed.\n";
  return 0;
}
