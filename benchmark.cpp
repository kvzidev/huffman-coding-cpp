#include <chrono>
#include <iomanip>

#include "huffman.hpp"

long getFileSize(const std::string& path) {
  FILE* file = fopen(path.c_str(), "rb");
  if (!file) {
    return 0;
  }

  fseek(file, 0, SEEK_END);
  long size = ftell(file);
  fclose(file);

  return size;
}

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

void runBenchmark(const std::string& benchmarkName, const void* data,
                  size_t dataSize) {
  std::string inputPath = benchmarkName + ".tmp";
  std::string compressedPath = benchmarkName + ".tmp.huf";
  std::string decompressedPath = benchmarkName + "_decompressed.tmp";

  FILE* inputFile = fopen(inputPath.c_str(), "wb");
  if (!inputFile) {
    std::cerr << "Failed to create benchmark input file: " << inputPath << "\n";
    return;
  }
  fwrite(data, sizeof(uint8_t), dataSize, inputFile);
  fclose(inputFile);

  long originalBytes = getFileSize(inputPath);
  auto silentProgress = [](int) {};

  auto compressionStartTime = std::chrono::high_resolution_clock::now();
  fCompress(inputPath, silentProgress);
  auto compressionEndTime = std::chrono::high_resolution_clock::now();

  long compressedBytes = getFileSize(compressedPath);

  auto decompressionStartTime = std::chrono::high_resolution_clock::now();
  fDecompress(compressedPath, silentProgress);
  auto decompressionEndTime = std::chrono::high_resolution_clock::now();

  std::chrono::duration<double, std::milli> compressionDuration =
      compressionEndTime - compressionStartTime;
  std::chrono::duration<double, std::milli> decompressionDuration =
      decompressionEndTime - decompressionStartTime;

  double originalMegabytes =
      static_cast<double>(originalBytes) / (1024.0 * 1024.0);
  double compressionThroughput =
      originalMegabytes / (compressionDuration.count() / 1000.0);
  double decompressionThroughput =
      originalMegabytes / (decompressionDuration.count() / 1000.0);

  double compressionPercentage = (static_cast<double>(compressedBytes) /
                                  static_cast<double>(originalBytes)) *
                                 100.0;
  double spaceSavingsPercentage = 100.0 - compressionPercentage;

  bool verified = compareFiles(inputPath, decompressedPath);

  std::cout << "------------------------------------------------------------\n";
  std::cout << "Benchmark: " << benchmarkName << "\n";
  std::cout << "------------------------------------------------------------\n";
  std::cout << std::fixed << std::setprecision(2);
  std::cout << "Original Size:          " << originalBytes << " bytes ("
            << originalMegabytes << " MB)\n";
  std::cout << "Compressed Size:        " << compressedBytes << " bytes\n";
  std::cout << "Space Savings:          " << spaceSavingsPercentage << "%\n";
  std::cout << "Compression Time:       " << compressionDuration.count()
            << " ms (" << compressionThroughput << " MB/s)\n";
  std::cout << "Decompression Time:     " << decompressionDuration.count()
            << " ms (" << decompressionThroughput << " MB/s)\n";
  std::cout << "Integrity Verification: " << (verified ? "PASSED" : "FAILED")
            << "\n\n";

  std::remove(inputPath.c_str());
  std::remove(compressedPath.c_str());
  std::remove(decompressedPath.c_str());
}

std::vector<uint8_t> generateRepetitiveData(size_t targetBytes) {
  std::string pattern = "ABCDEFGHABCDEFGH";
  std::vector<uint8_t> data(targetBytes);
  for (size_t i = 0; i < targetBytes; i++) {
    data[i] = static_cast<uint8_t>(pattern[i % pattern.size()]);
  }
  return data;
}

std::vector<uint8_t> generateNaturalTextData(size_t targetBytes) {
  std::string sampleText =
      "Data structures and algorithms form the foundation of computer science. "
      "Huffman coding is a greedy algorithm that builds optimal prefix codes "
      "based on "
      "symbol frequency distributions. Frequent symbols receive short codes, "
      "while rare "
      "symbols receive long codes, minimizing expected bit length. ";

  std::vector<uint8_t> data(targetBytes);
  for (size_t i = 0; i < targetBytes; i++) {
    data[i] = static_cast<uint8_t>(sampleText[i % sampleText.size()]);
  }
  return data;
}

std::vector<uint8_t> generatePseudorandomData(size_t targetBytes) {
  std::vector<uint8_t> data(targetBytes);
  uint32_t state = 123456789;
  for (size_t i = 0; i < targetBytes; i++) {
    state = state * 1664525 + 1013904223;
    data[i] = static_cast<uint8_t>((state >> 16) & 0xFF);
  }
  return data;
}

int main() {
  size_t payloadBytes = 256 * 1024;

  std::cout << "============================================================\n";
  std::cout << "           Huffman Coding Performance Benchmark             \n";
  std::cout
      << "============================================================\n\n";

  std::vector<uint8_t> repetitiveData = generateRepetitiveData(payloadBytes);
  runBenchmark("Low_Entropy_Repetitive", repetitiveData.data(),
               repetitiveData.size());

  std::vector<uint8_t> textData = generateNaturalTextData(payloadBytes);
  runBenchmark("Medium_Entropy_Natural_Text", textData.data(), textData.size());

  std::vector<uint8_t> randomData = generatePseudorandomData(payloadBytes);
  runBenchmark("High_Entropy_Pseudorandom", randomData.data(),
               randomData.size());

  return 0;
}
