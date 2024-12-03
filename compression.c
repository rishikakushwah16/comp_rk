/*
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define block size
#define BLOCK_SIZE 128

// Utility function to compute integer base-2 logarithm
unsigned integerLog2(unsigned x) {
    unsigned result = 0;
    while (x >>= 1) {
        result++;
    }
    return result;
}

// Global counters for function calls
static unsigned long long convertBuffer2Array_count = 0;
static unsigned long long isZeroPackable_count = 0;
static unsigned long long isZeroPackable_true = 0;
static unsigned long long isSameValuePackable_count = 0;
static unsigned long long isSameValuePackable_true = 0;
static unsigned long long multBaseCompression_count = 0;
static unsigned long long multBaseCompression_success = 0;
static unsigned long long BDICompress_count = 0;
/*
// Function to convert buffer to an array of values
long long unsigned* convertBuffer2Array(char* buffer, unsigned size, unsigned step) {
    convertBuffer2Array_count++;
    unsigned num_values = size / step;
    long long unsigned* values = (long long unsigned*)malloc(sizeof(long long unsigned) * num_values);
    if (!values) {
        fprintf(stderr, "Memory allocation failed in convertBuffer2Array\n");
        exit(1);
    }

    for (unsigned int i = 0; i < num_values; i++) {
        values[i] = 0;
        for (unsigned int j = 0; j < step; j++) {
            values[i] += (long long unsigned)((unsigned char)buffer[i * step + j]) << (8 * j);
        }
    }
    return values;
}

long long unsigned* convertBuffer2Array(char* buffer, unsigned size, unsigned step) {
    convertBuffer2Array_count++;
    unsigned num_values = (size * 2) / step;  // Each byte holds two 4-bit values.
    long long unsigned* values = (long long unsigned*)malloc(sizeof(long long unsigned) * num_values);
    if (!values) {
        fprintf(stderr, "Memory allocation failed in convertBuffer2Array\n");
        exit(EXIT_FAILURE);
    }

    for (unsigned int i = 0; i < num_values; i++) {
        values[i] = (i % 2 == 0) ? (buffer[i / 2] & 0x0F) : ((buffer[i / 2] >> 4) & 0x0F);
    }
    return values;
}

// Check if the cache line consists of only zero values
int isZeroPackable(long long unsigned* values, unsigned size) {
    isZeroPackable_count++;
    for (unsigned int i = 0; i < size; i++) {
        if (values[i] != 0) {
            return 0;  // Not zero-packable
        }
    }
    isZeroPackable_true++;
    return 1;  // Zero-packable
}

// Check if the cache line consists of only same values
int isSameValuePackable(long long unsigned* values, unsigned size) {
    isSameValuePackable_count++;
    for (unsigned int i = 1; i < size; i++) {
        if (values[0] != values[i]) {
            return 0;  // Not same-value-packable
        }
    }
    isSameValuePackable_true++;
    return 1;  // Same-value-packable
}
/*
// Modified multBaseCompression function using single base (first value)
unsigned multBaseCompression_singleBase(long long unsigned* values, unsigned size) {
    multBaseCompression_count++;

    unsigned char base = (unsigned char)values[0]; // Use the first value as the base

    // Determine the maximal absolute delta
    unsigned maxAbsDelta = 0;
    for (unsigned int i = 0; i < size; i++) {
        int delta = (int)((unsigned char)values[i]) - base;
        if (delta < -128 || delta > 127) {
            // Delta does not fit within bounds; cannot compress
            return size;  // Return original size (uncompressed)
        }
        unsigned absDelta = delta < 0 ? -delta : delta;
        if (absDelta > maxAbsDelta) {
            maxAbsDelta = absDelta;
        }
    }

    // Decide delta size based on maxAbsDelta
    unsigned deltaBits = 0;
    if (maxAbsDelta == 0) {
        deltaBits = 1;  // Only need 1 bit to represent zero delta
    } else {
        deltaBits = integerLog2(maxAbsDelta) + 1 + 1;  // +1 for sign bit
    }

    // Calculate compressed size
    unsigned totalBits = 8;                           // Bits for the base
    totalBits += size * deltaBits;                    // Bits for deltas

    // Convert bits to bytes, rounding up
    unsigned compSize = (totalBits + 7) / 8;

    if (compSize < size) {
        multBaseCompression_success++;
        return compSize;
    } else {
        return size;  // Return original size if compression is not beneficial
    }
}

unsigned multBaseCompression_singleBase(long long unsigned* values, unsigned size) {
    multBaseCompression_count++;

    unsigned char base = (unsigned char)values[0]; // Use the first value as the base

    // Determine the maximal absolute delta
    unsigned maxAbsDelta = 0;
    for (unsigned int i = 0; i < size; i++) {
        int delta = (int)((unsigned char)values[i]) - base;
        if (delta < -7 || delta > 7) {  // Adjusted range for 4-bit signed values
            // Delta does not fit within 4-bit bounds; cannot compress
            return size;  // Return original size (uncompressed)
        }
        unsigned absDelta = delta < 0 ? -delta : delta;
        if (absDelta > maxAbsDelta) {
            maxAbsDelta = absDelta;
        }
    }

    // The delta size is always 4 bits for 4-bit quantized values
    unsigned deltaBits = 4;

    // Calculate compressed size
    unsigned totalBits = 4;                         // Bits for the base (4 bits)
    totalBits += size * deltaBits;                  // Bits for deltas (4 bits per value)

    // Convert bits to bytes, rounding up
    unsigned compSize = (totalBits + 7) / 8;

    if (compSize < size) {
        multBaseCompression_success++;
        return compSize;
    } else {
        return size;  // Return original size if compression is not beneficial
    }
}


// BDI Compression function
unsigned BDICompress(char* buffer, unsigned _blockSize) {
    BDICompress_count++;
    unsigned bestCSize = _blockSize;
    unsigned currCSize = _blockSize;
    unsigned size = _blockSize;

    // Process as 1-byte values
    long long unsigned* values = convertBuffer2Array(buffer, size, 1);

    // Check if zero-packable
    if (isZeroPackable(values, size)) {
        bestCSize = 1;  // Only need to store the base (zero)
        free(values);
        return bestCSize;
    }

    // Check if same-value-packable
    if (isSameValuePackable(values, size)) {
        currCSize = 1;  // Only need to store the base value
        bestCSize = bestCSize > currCSize ? currCSize : bestCSize;
    }

    // Attempt compression using the first value as the base
    currCSize = multBaseCompression_singleBase(values, size);
    bestCSize = bestCSize > currCSize ? currCSize : bestCSize;

    free(values);
    return bestCSize;
}

// General compression function
unsigned GeneralCompress(char* buffer, unsigned _blockSize, unsigned compress) {
    switch (compress) {
        case 0:
            return _blockSize;
        case 1:
            return BDICompress(buffer, _blockSize);
        default:
            fprintf(stderr, "Unknown compression code: %u\n", compress);
            exit(1);
    }
}

// Main function to compress the binary file
int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <binary_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* input_file = argv[1];

    FILE* infile = fopen(input_file, "rb");
    if (!infile) {
        fprintf(stderr, "Error: Cannot open file %s\n", input_file);
        return EXIT_FAILURE;
    }

    unsigned long long total_original_size = 0;
    unsigned long long total_compressed_size = 0;
    char buffer[BLOCK_SIZE];

    while (1) {
        size_t bytes_read = fread(buffer, 1, BLOCK_SIZE, infile);
        if (bytes_read == 0) {
            break;
        }
        // If the last block is smaller than BLOCK_SIZE, pad it with zeros
        if (bytes_read < BLOCK_SIZE) {
            memset(buffer + bytes_read, 0, BLOCK_SIZE - bytes_read);
        }
        total_original_size += BLOCK_SIZE;
        unsigned compressed_size = GeneralCompress(buffer, BLOCK_SIZE, 1);  // Using BDI compression
        total_compressed_size += compressed_size;
    }

    fclose(infile);

    // Calculate compression ratio
    double ratio = (double)total_original_size / (double)total_compressed_size;
    if (total_compressed_size == 0) {
        fprintf(stderr, "Warning: Compressed size is zero, ratio calculation not possible.\n");
    } else {
        printf("Compression Ratio: %.2f:1\n", ratio);
    }
    printf("Original Size: %llu bytes\n", total_original_size);
    printf("Compressed Size: %llu bytes\n", total_compressed_size);
    printf("Compression Ratio: %.2f:1\n", ratio);

    // Print function call distribution
    printf("\nFunction Call Distribution:\n");
    printf("convertBuffer2Array called: %llu times\n", convertBuffer2Array_count);
    printf("isZeroPackable called: %llu times, true: %llu times\n", isZeroPackable_count, isZeroPackable_true);
    printf("isSameValuePackable called: %llu times, true: %llu times\n", isSameValuePackable_count, isSameValuePackable_true);
    printf("multBaseCompression called: %llu times, success: %llu times\n", multBaseCompression_count, multBaseCompression_success);
    printf("BDICompress called: %llu times\n", BDICompress_count);

    return EXIT_SUCCESS;
}
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Define block size
#define BLOCK_SIZE 64

// Utility function to compute integer base-2 logarithm
unsigned integerLog2(unsigned x) {
    unsigned result = 0;
    while (x >>= 1) {
        result++;
    }
    return result;
}

// Global counters for function calls
static unsigned long long convertBuffer2Array_count = 0;
static unsigned long long isZeroPackable_count = 0;
static unsigned long long isZeroPackable_true = 0;
static unsigned long long isSameValuePackable_count = 0;
static unsigned long long isSameValuePackable_true = 0;
static unsigned long long multBaseCompression_count = 0;
static unsigned long long multBaseCompression_success = 0;
static unsigned long long BDICompress_count = 0;

// Function to convert buffer to an array of 16-bit values for FP16
unsigned short* convertBuffer2Array(char* buffer, unsigned size, unsigned step) {
    convertBuffer2Array_count++;
    unsigned num_values = size / step;
    unsigned short* values = (unsigned short*)malloc(sizeof(unsigned short) * num_values);
    if (!values) {
        fprintf(stderr, "Memory allocation failed in convertBuffer2Array\n");
        exit(EXIT_FAILURE);
    }

    for (unsigned int i = 0; i < num_values; i++) {
        values[i] = (unsigned short)(((unsigned char)buffer[i * 2]) | (((unsigned char)buffer[i * 2 + 1]) << 8));
    }
    return values;
}

// Check if the cache line consists of only zero values
int isZeroPackable(unsigned short* values, unsigned size) {
    isZeroPackable_count++;
    for (unsigned int i = 0; i < size; i++) {
        if (values[i] != 0) {
            return 0;  // Not zero-packable
        }
    }
    isZeroPackable_true++;
    return 1;  // Zero-packable
}

// Check if the cache line consists of only same values
int isSameValuePackable(unsigned short* values, unsigned size) {
    isSameValuePackable_count++;
    for (unsigned int i = 1; i < size; i++) {
        if (values[0] != values[i]) {
            return 0;  // Not same-value-packable
        }
    }
    isSameValuePackable_true++;
    return 1;  // Same-value-packable
}

// Modified multBaseCompression function using single base (first value)
unsigned multBaseCompression_singleBase(unsigned short* values, unsigned size) {
    multBaseCompression_count++;

    unsigned short base = values[0]; // Use the first value as the base

    // Determine the maximal absolute delta
    unsigned maxAbsDelta = 0;
    for (unsigned int i = 0; i < size; i++) {
        int delta = (int)values[i] - base;
        if (delta < -32768 || delta > 32767) {  // Adjusted range for 16-bit signed values
            // Delta does not fit within 16-bit bounds; cannot compress
            return size * 2;  // Return original size (uncompressed)
        }
        unsigned absDelta = delta < 0 ? -delta : delta;
        if (absDelta > maxAbsDelta) {
            maxAbsDelta = absDelta;
        }
    }

    // The delta size is 16 bits for FP16
    unsigned deltaBits = 16;

    // Calculate compressed size
    unsigned totalBits = 16;                         // Bits for the base (16 bits)
    totalBits += size * deltaBits;                   // Bits for deltas (16 bits per value)

    // Convert bits to bytes, rounding up
    unsigned compSize = (totalBits + 7) / 8;

    if (compSize < size * 2) {
        multBaseCompression_success++;
        return compSize;
    } else {
        return size * 2;  // Return original size if compression is not beneficial
    }
}

// BDI Compression function
unsigned BDICompress(char* buffer, unsigned _blockSize) {
    BDICompress_count++;
    unsigned bestCSize = _blockSize;
    unsigned currCSize = _blockSize;
    unsigned size = _blockSize / 2;  // FP16 data, 2 bytes per value

    // Process as 16-bit values
    unsigned short* values = convertBuffer2Array(buffer, _blockSize, 2);

    // Check if zero-packable
    if (isZeroPackable(values, size)) {
        bestCSize = 2;  // Only need to store the base (zero)
        free(values);
        return bestCSize;
    }

    // Check if same-value-packable
    if (isSameValuePackable(values, size)) {
        currCSize = 2;  // Only need to store the base value
        bestCSize = bestCSize > currCSize ? currCSize : bestCSize;
    }

    // Attempt compression using the first value as the base
    currCSize = multBaseCompression_singleBase(values, size);
    bestCSize = bestCSize > currCSize ? currCSize : bestCSize;

    free(values);
    return bestCSize;
}

// General compression function
unsigned GeneralCompress(char* buffer, unsigned _blockSize, unsigned compress) {
    switch (compress) {
        case 0:
            return _blockSize;
        case 1:
            return BDICompress(buffer, _blockSize);
        default:
            fprintf(stderr, "Unknown compression code: %u\n", compress);
            exit(1);
    }
}

// Main function to compress the binary file
int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <binary_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char* input_file = argv[1];

    FILE* infile = fopen(input_file, "rb");
    if (!infile) {
        fprintf(stderr, "Error: Cannot open file %s\n", input_file);
        return EXIT_FAILURE;
    }

    unsigned long long total_original_size = 0;
    unsigned long long total_compressed_size = 0;
    char buffer[BLOCK_SIZE];

    while (1) {
        size_t bytes_read = fread(buffer, 1, BLOCK_SIZE, infile);
        if (bytes_read == 0) {
            break;
        }
        // If the last block is smaller than BLOCK_SIZE, pad it with zeros
        if (bytes_read < BLOCK_SIZE) {
            memset(buffer + bytes_read, 0, BLOCK_SIZE - bytes_read);
        }
        total_original_size += BLOCK_SIZE;
        unsigned compressed_size = GeneralCompress(buffer, BLOCK_SIZE, 1);  // Using BDI compression
        total_compressed_size += compressed_size;
    }

    fclose(infile);

    // Calculate compression ratio
    double ratio = (double)total_original_size / (double)total_compressed_size;
    if (total_compressed_size == 0) {
        fprintf(stderr, "Warning: Compressed size is zero, ratio calculation not possible.\n");
    } else {
        printf("Compression Ratio: %.2f:1\n", ratio);
    }
    printf("Original Size: %llu bytes\n", total_original_size);
    printf("Compressed Size: %llu bytes\n", total_compressed_size);
    printf("Compression Ratio: %.2f:1\n", ratio);

    // Print function call distribution
    printf("\nFunction Call Distribution:\n");
    printf("convertBuffer2Array called: %llu times\n", convertBuffer2Array_count);
    printf("isZeroPackable called: %llu times, true: %llu times\n", isZeroPackable_count, isZeroPackable_true);
    printf("isSameValuePackable called: %llu times, true: %llu times\n", isSameValuePackable_count, isSameValuePackable_true);
    printf("multBaseCompression called: %llu times, success: %llu times\n", multBaseCompression_count, multBaseCompression_success);
    printf("BDICompress called: %llu times\n", BDICompress_count);

    return EXIT_SUCCESS;

}