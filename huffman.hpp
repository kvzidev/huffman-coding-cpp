#ifndef HUFFMAN_H
#define HUFFMAN_H

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <list>
#include <vector>

// ============================================================================================================
// = STRUCTS
// ==================================================================================================
// ============================================================================================================

struct InfoByte {
  uint8_t symbol;
  uint64_t n;              // Amount of times each byte is repeated
  uint64_t vsize;          // Size of the vector
  std::vector<bool> code;  // Code
};

struct HuffmanTreeNode {
  uint8_t symbol;
  uint64_t freq;
  HuffmanTreeNode* left;
  HuffmanTreeNode* right;

  HuffmanTreeNode() : symbol('\0'), freq(0), left(nullptr), right(nullptr) {};
  HuffmanTreeNode(uint8_t symbol, uint64_t freq,
                  HuffmanTreeNode* left = nullptr,
                  HuffmanTreeNode* right = nullptr)
      : symbol(symbol), freq(freq), left(left), right(right) {};

  bool operator<(const HuffmanTreeNode& other) const {
    return freq < other.freq;
  }
};

struct HuffmanTreeInfo {
  uint64_t numNodes;
  uint64_t depth;

  HuffmanTreeInfo(uint64_t numNodes, uint64_t depth)
      : numNodes(numNodes), depth(depth) {};
};

class BitWriter {
 private:
  FILE*& outputFile;
  uint8_t buffer;
  int count;
  uint32_t bitLength;

 public:
  BitWriter(FILE*& outputFile)
      : outputFile(outputFile), buffer(0), count(0), bitLength(0) {}

  void writeBit(bool bit) {
    buffer <<= 1;
    buffer |= bit;
    count++;
    if (count == 8) saveBuffer();
  }

  void saveBuffer() {
    fwrite(&buffer, sizeof(buffer), 1, outputFile);
    buffer = 0;
    count = 0;
  }

  void flush() {
    if (count > 0) {
      buffer <<= (8 - count);
      saveBuffer();
    }
  }

  void writeBitLength() { fwrite(&bitLength, sizeof(uint32_t), 1, outputFile); }

  void setBitLength(uint32_t length) { bitLength = length; }
};

// ============================================================================================================
// = UTILS
// ==============================================================================================
// ============================================================================================================

void destroyTree(HuffmanTreeNode* node) {
  if (!node) return;
  destroyTree(node->left);
  destroyTree(node->right);
  delete node;
}

// ============================================================================================================
// = COMPRESSION
// ==============================================================================================
// ============================================================================================================

bool fByteCounter(std::string& path, std::vector<InfoByte>& arr) {
  FILE* f = fopen(path.c_str(), "r+b");

  if (!f) {
    std::cerr << "Failed to open " << path << std::endl;
    return 1;
  }

  // Count
  uint8_t symbol;
  while (fread(&symbol, sizeof(symbol), 1, f)) {
    if (symbol >= arr.size()) {
      int i = arr.size();
      while (i <= symbol) {
        arr.push_back({0, 0, 0, {}});
        i++;
      }
    }
    arr[symbol].n++;
    arr[symbol].symbol = symbol;
  }

  // Close file
  fclose(f);
  return 0;
}

std::list<HuffmanTreeNode*> fOrderedList(std::vector<InfoByte>& arr) {
  std::list<HuffmanTreeNode*> list = std::list<HuffmanTreeNode*>();
  for (size_t i = 0; i < arr.size(); i++) {
    if (arr[i].n > 0) {
      uint8_t symbol = i;
      HuffmanTreeNode* node = new HuffmanTreeNode(symbol, arr[i].n);
      auto it = std::upper_bound(
          list.begin(), list.end(), node,
          [](const HuffmanTreeNode* left, const HuffmanTreeNode* right) {
            return left->freq < right->freq;
          });
      list.insert(it, node);
    }
  }

  return list;
}

HuffmanTreeNode* fListToTree(std::list<HuffmanTreeNode*> list) {
  if (list.empty()) return nullptr;

  if (list.size() == 1) {
    HuffmanTreeNode* root = list.front();
    return new HuffmanTreeNode('\0', root->freq, root, nullptr);
  }

  // Loop until there is only one node left in the list
  while (list.size() > 1) {
    // Pop the first two nodes from the list
    HuffmanTreeNode* node1 = list.front();
    list.pop_front();
    HuffmanTreeNode* node2 = list.front();
    list.pop_front();

    // Create a new node with the sum of the frequencies of node1 and node2
    HuffmanTreeNode* newNode =
        new HuffmanTreeNode('\0', node1->freq + node2->freq, node1, node2);

    // Insert the new node back into the list, maintaining the order
    auto it = std::upper_bound(
        list.begin(), list.end(), newNode,
        [](const HuffmanTreeNode* left, const HuffmanTreeNode* right) {
          return left->freq < right->freq;
        });
    list.insert(it, newNode);
  }

  // Return the last node in the list as the root of the tree
  return list.front();
}

bool _fGenerateHuffmanCode(HuffmanTreeNode* node, std::vector<bool>& code,
                           std::vector<InfoByte>& arr) {
  if (!node) return false;

  // Leaf
  if (node->left == nullptr && node->right == nullptr) {
    if (node->symbol >= arr.size()) return false;

    arr[node->symbol].code = code;
    arr[node->symbol].vsize = code.size();
    return true;
  }

  code.push_back(false);  // 0 is left
  _fGenerateHuffmanCode(node->left, code, arr);
  code.pop_back();  // Removes the last false as it is nullptr

  code.push_back(true);
  _fGenerateHuffmanCode(node->right, code, arr);
  code.pop_back();  // Removes the last true as it is nullptr

  return true;
}

bool fGenerateHuffmanCode(HuffmanTreeNode*& root, std::vector<InfoByte>& arr) {
  std::vector<bool> code;

  if (!root) return false;

  _fGenerateHuffmanCode(root, code, arr);

  return true;
}

void fWriteHeader(FILE*& outputFile, std::vector<InfoByte>& arr) {
  uint16_t activeSymbolCount = 0;
  for (const auto& byteInfo : arr) {
    if (byteInfo.n > 0) {
      activeSymbolCount++;
    }
  }

  // Write the array
  fwrite(&activeSymbolCount, sizeof(uint16_t), 1, outputFile);

  // Write the frequency table to the output file in binary format
  for (const auto& byteInfo : arr) {
    if (byteInfo.n > 0) {
      fwrite(&byteInfo.symbol, sizeof(uint8_t), 1, outputFile);
      fwrite(&byteInfo.n, sizeof(uint64_t), 1, outputFile);
    }
  }
}

void fSaveCompressedFile(std::string& path, std::vector<InfoByte>& arr) {
  std::string outputPath = path + ".huf";

  FILE* inputFile = fopen(path.c_str(), "rb");

  if (inputFile == nullptr) {
    std::cerr << "Failed to open input file." << std::endl;
    return;
  }

  FILE* outputFile = fopen(outputPath.c_str(), "wb");
  if (outputFile == nullptr) {
    std::cerr << "Failed to output input file." << std::endl;
    fclose(inputFile);
    return;
  }

  // Write header
  fWriteHeader(outputFile, arr);

  // Create a BitWriter object to write the encoded data to the output file
  BitWriter bitWriter(outputFile);

  // Go to the start of the input file
  fseek(inputFile, 0, SEEK_SET);

  // Encode each symbol in the input file using Huffman coding
  uint8_t symbol;
  while (fread(&symbol, sizeof(uint8_t), 1, inputFile) == 1) {
    // Get the Huffman code for the symbol
    const std::vector<bool>& code = arr[symbol].code;
    for (bool bit : code) {
      bitWriter.writeBit(bit);
    }
  }

  // Flush any remaining bits in the BitWriter buffer to the output file
  bitWriter.flush();

  // Close the input and output files
  fclose(inputFile);
  fclose(outputFile);
}

void fCompress(std::string& path, void (*fDisplayProgress)(int)) {
  // Declaring the array for the bytes
  std::vector<InfoByte> arr;

  // Step 1: Count how many times each byte appears
  fByteCounter(path, arr);

  fDisplayProgress(20);  // Step 1 of 5

  // Step 2: Create a list and order it
  std::list<HuffmanTreeNode*> list = fOrderedList(arr);

  fDisplayProgress(40);  // Step 2 of 5

  if (list.empty()) {
    fDisplayProgress(80);
    fSaveCompressedFile(path, arr);
    fDisplayProgress(100);
    return;
  }

  // Step 3: Convert the list into a tree
  HuffmanTreeNode* root = fListToTree(list);

  fDisplayProgress(60);  // Step 3 of 5

  // Step 4: Go through the tree and save each code inside an array
  fGenerateHuffmanCode(root, arr);
  destroyTree(root);

  fDisplayProgress(80);  // Step 4 of 5

  // Step 5: Go through the original files and save each bit in the compressed
  // file
  fSaveCompressedFile(path, arr);

  fDisplayProgress(100);  // Step 5 of 5
}

// ============================================================================================================
// = DECOMPRESSION
// ============================================================================================
// ============================================================================================================

HuffmanTreeNode* fRebuildTree(FILE*& f, uint64_t& totalBytes) {
  uint16_t activeSymbolCount = 0;
  if (fread(&activeSymbolCount, sizeof(uint16_t), 1, f) != 1) {
    return nullptr;
  }

  totalBytes = 0;
  std::vector<InfoByte> arr(256, {0, 0, 0, {}});

  for (int i = 0; i < activeSymbolCount; i++) {
    uint8_t symbol = 0;
    uint64_t freq = 0;

    fread(&symbol, sizeof(uint8_t), 1, f);
    fread(&freq, sizeof(uint64_t), 1, f);

    arr[symbol].symbol = symbol;
    arr[symbol].n = freq;
    totalBytes += freq;
  }

  std::list<HuffmanTreeNode*> list = fOrderedList(arr);
  return fListToTree(list);
}

void fReadDecodeCreate(FILE*& f, HuffmanTreeNode*& root, std::string& path,
                       uint64_t totalBytes) {
  // Open the output file for writing
  std::string basePath = path.substr(0, path.length() - 4);
  size_t lastDotIndex = basePath.rfind('.');
  size_t lastSlashIndex = basePath.find_last_of("/\\");

  std::string outputPath;
  if (lastDotIndex != std::string::npos &&
      (lastSlashIndex == std::string::npos || lastDotIndex > lastSlashIndex)) {
    outputPath = basePath.substr(0, lastDotIndex) + "_decompressed" +
                 basePath.substr(lastDotIndex);
  } else {
    outputPath = basePath + "_decompressed";
  }

  FILE* outputFile = fopen(outputPath.c_str(), "wb");
  if (outputFile == nullptr) {
    std::cerr << "Failed to output input file." << std::endl;
    return;
  }

  if (totalBytes == 0) {
    fclose(outputFile);
    return;
  }

  // Traverse the Huffman tree to decode the input data
  HuffmanTreeNode* node = root;
  uint64_t decodedBytes = 0;
  uint8_t byte;

  while (decodedBytes < totalBytes &&
         fread(&byte, sizeof(uint8_t), 1, f) == 1) {
    for (int bitIndex = 7; bitIndex >= 0 && decodedBytes < totalBytes;
         bitIndex--) {
      bool bit = ((byte >> bitIndex) & 1);

      if (bit) {
        node = node->right;
      } else {
        node = node->left;
      }

      if (node->left == nullptr && node->right == nullptr) {
        // Found a leaf node, so write the symbol to the output file
        fwrite(&node->symbol, sizeof(uint8_t), 1, outputFile);
        decodedBytes++;

        // Reset the Huffman tree traversal to the root node
        node = root;
      }
    }
  }

  // Close the output file
  fclose(outputFile);
}

void fDecompress(std::string& path, void (*fDisplayProgress)(int)) {
  // Step 1: Open the file
  FILE* f = fopen(path.c_str(), "r+b");

  if (!f) {
    std::cerr << "Failed to open file for reading." << std::endl;
    return;
  }

  fDisplayProgress(25);  // Step 1 of 4

  // Step 2: Rebuild the tree
  uint64_t totalBytes = 0;
  HuffmanTreeNode* root = fRebuildTree(f, totalBytes);

  fDisplayProgress(50);  // Step 2 of 4

  // Step 3: Read encoded info and decode into a new file
  fReadDecodeCreate(f, root, path, totalBytes);
  destroyTree(root);

  fDisplayProgress(75);  // Step 3 of 4

  // Step 4: Safely close the file
  fclose(f);

  fDisplayProgress(100);  // Step 4 of 4
}

#endif
