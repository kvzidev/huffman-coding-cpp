# Huffman Coding Compressor/Decompressor

Implementation of the Huffman Coding for lossless data compression and decompression. It was developed as a university assignment to demostrate fundamental concepts of data compression algorithms and tree data structures.

## Features

1. **Compress File:** Takes an input text file, applies Huffman coding to it, and generates a compressed binary output.
2. **Decompress File:** Takes a compresed Huffman-encoded file and reconstructs the original text file.

## How Huffman Coding Works (Briefly)

Huffman coding is a variable-length prefix coding technique. It works by:

- Counting Frequencies: Determining how often each character appears in the input data.
- Building a Tree: Constructing a binary tree (the Huffman tree) where characters with lower frequencies are further down the tree (resulting in longer codes), and characters with higher frequencies are closer to the root (resulting in shorter codes).
- Generating Codes: Assigning unique binary codes (sequences of 0s and 1s) to each character by traversing the tree from the root to the leaf node where the character resides. These codes are "prefix-free," meaning no character's code is a prefix of another character's code, which allows unambiguous decoding.
- Encoding: Replacing each character in the input data with its corresponding Huffman code.
- Decoding: Using the Huffman tree to read the binary stream and reconstruct the original characters.

## Usage

You need a C++ compiler to run this.

1. Clone the repo
    ```sh
    git clone https://github.com/kizzandev/huffman-coding-cpp.git
    cd huffman-coding-cpp
    ```

2. Compile, e.g. with gcc
    ```sh
    g++ main.cpp -o huffman_codec
    ```

3. After compilling, run the executable
    ```sh
    ./huffman_codec
    ```

## Contributing

As this was an university assignment, contributions are not expected. However, if you find bugs or have suggestions for minor improvements (e.g. code clarity, better error handling), feel free to open a pull request.
