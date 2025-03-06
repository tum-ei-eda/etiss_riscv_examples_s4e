#include <stdio.h>
#include <memory.h>
#include <stdint.h>

typedef unsigned char bool;

uint32_t v[16];

static inline uint32_t rol(uint32_t a, int n) {
    return (a << n) | (a >> (32 - n));
}

static inline uint32_t xor(uint32_t a, uint32_t b) {
    return a ^ b;
}


static inline uint32_t rolxor(uint32_t a, uint32_t b, int n) {
    return rol(xor(a, b), n);
}

//#define CHACHA_EXTENSION

#ifndef CHACHA_EXTENSION

#pragma message "Building with pure C code"

static inline void quarter_round(int a, int b, int c, int d) {
    v[a] = v[a] + v[b];
    v[d] = rolxor(v[d], v[a], 16);
    v[c] = v[c] + v[d];
    v[b] = rolxor(v[b], v[c], 12);
    v[a] = v[a] + v[b];
    v[d] = rolxor(v[d], v[a],  8);
    v[c] = v[c] + v[d];
    v[b] = rolxor(v[b], v[c],  7);
}

#else

#pragma message "Building with CHACHA extension"

#define USE_INTRINSICS

#ifdef USE_INTRINSICS

//#error DON'T USE THIS PATHWAY

static inline void quarter_round(int a, int b, int c, int d) {
    v[a] = v[a] + v[b];
    v[d] = __builtin_xexample_chacha20_qr1(v[d], v[a]);
    v[c] = v[c] + v[d];
    v[b] = __builtin_xexample_chacha20_qr2(v[b], v[c]);
    v[a] = v[a] + v[b];
    v[d] = __builtin_xexample_chacha20_qr3(v[d], v[a]);
    v[c] = v[c] + v[d];
    v[b] = __builtin_xexample_chacha20_qr4(v[b], v[c]);
}

#else

#error "Assembler version not yet written"

#endif

#endif


void double_rounds(uint32_t output[16], const uint32_t input[16]) {
    memcpy(v, input, sizeof(v));

    for(int i = 0; i < 20; i += 2) {
        quarter_round( 0, 4, 8,12);
        quarter_round( 1, 5, 9,13);
        quarter_round( 2, 6,10,14);
        quarter_round( 3, 7,11,15);

        quarter_round( 0, 5,10,15);
        quarter_round( 1, 6,11,12);
        quarter_round( 2, 7, 8,13);
        quarter_round( 3, 4, 9,14);
    }

    for(int i = 0; i < 16; ++i) {
        v[i] += input[i];
        output[i] = v[i];
    }
}


// Tests a single quarter round, using the sample numbers given in
// in section 2.1.1 of the ChaCha20 RFC
// https://datatracker.ietf.org/doc/html/rfc8439
bool test_quarter_round() {
    v[0] = 0x11111111;
    v[1] = 0x01020304;
    v[2] = 0x9b8d6f43;
    v[3] = 0x01234567;
    printf("Testing quarter_round()\n"
           "  a = %08x\n"
           "  b = %08x\n"
           "  c = %08x\n"
           "  d = %08x\n", v[0], v[1], v[2], v[3]);

    quarter_round(0, 1, 2, 3);

    printf("Updated values:\n"
           "  a = %08x\n"
           "  b = %08x\n"
           "  c = %08x\n"
           "  d = %08x\n", v[0], v[1], v[2], v[3]);

    bool passed =
        v[0] == 0xea2a92f4 &&
        v[1] == 0xcb1cf8ce &&
        v[2] == 0x4581472e &&
        v[3] == 0x5881c4bb;
    printf("test_quarter_round() %s\n", passed? "PASSED": "FAILED");
    return passed;
}

// Generates the keystream for the first 64-byte block
// from a 256-bit key with all IV fields other than 16 bytes
// of constant "expand 32-byte k"
// The resulting block is compared to the test vector taken from
// page 7 of
// https://www.ietf.org/archive/id/draft-strombergson-chacha-test-vectors-01.txt
bool test_chacha_block() {
    // Initialise state with standard 256-bit-key constant and otherwise zero
    uint32_t iv[16];
    iv[0] = 0x61707865;
    iv[1] = 0x3320646e;
    iv[2] = 0x79622d32;
    iv[3] = 0x6b206574;
    memset(iv + 4, 0, sizeof(iv) - 16);

#ifdef USE_INTRINSICS
    printf("*** INTRINSIC FUNCTIONS IMPL ***\n");
#else
    printf("*** SIMPLE C FUNCTIONS IMPL ***\n");
#endif
    uint64_t start_tick = __builtin_readcyclecounter();
    uint32_t cipher_state[16];
    double_rounds(cipher_state, iv);
    uint64_t end_tick = __builtin_readcyclecounter();

    printf("Keystream:\n");
    // presumes little-endian
    uint8_t* ks = (uint8_t*) cipher_state;
    for(int r = 0; r < 8; ++r) {
        for(int c = 0; c < 8; ++c) {
            printf("%02x ", *ks++);
        }
        printf("\n");
    }

    //
    const uint8_t test_ks[] = {
        0x76, 0xb8, 0xe0, 0xad, 0xa0, 0xf1, 0x3d, 0x90,
        0x40, 0x5d, 0x6a, 0xe5, 0x53, 0x86, 0xbd, 0x28,
        0xbd, 0xd2, 0x19, 0xb8, 0xa0, 0x8d, 0xed, 0x1a,
        0xa8, 0x36, 0xef, 0xcc, 0x8b, 0x77, 0x0d, 0xc7,
        0xda, 0x41, 0x59, 0x7c, 0x51, 0x57, 0x48, 0x8d,
        0x77, 0x24, 0xe0, 0x3f, 0xb8, 0xd8, 0x4a, 0x37,
        0x6a, 0x43, 0xb8, 0xf4, 0x15, 0x18, 0xa1, 0x1c,
        0xc3, 0x87, 0xb6, 0x69, 0xb2, 0xee, 0x65, 0x86
    };
    bool passed = memcmp(cipher_state, test_ks, sizeof(test_ks)) == 0;
    printf("test_quarter_round() %s [%llu ticks]\n", passed? "PASSED": "FAILED", end_tick - start_tick);
    return passed;
}



int main()
{
    //test_quarter_round();

    test_chacha_block();

    return 0;
}
