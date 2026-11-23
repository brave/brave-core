/* Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0 OR ISC OR MIT-0
 */

/* ----------------------------------------------------------------------------
 * C prototypes for s2n-bignum functions, so you can use them in C programs via
 *
 *  #include "s2n-bignum-c89.h"
 *
 * The functions are listed in alphabetical order with a brief description
 * in comments for each one. For more detailed documentation see the comment
 * banner at the top of the corresponding assembly (.S) file, and
 * for the last word in what properties it satisfies see the spec in the
 * formal proof (the .ml file in the architecture-specific directory).
 *
 * For some functions there are additional variants with names ending in
 * "_alt". These have the same core mathematical functionality as their
 * non-"alt" versions, but can be better suited to some microarchitectures:
 *
 *      - On x86, the "_alt" forms avoid BMI and ADX instruction set
 *        extensions, so will run on any x86_64 machine, even older ones
 *
 *      - On ARM, the "_alt" forms target machines with higher multiplier
 *        throughput, generally offering higher performance there.
 * ----------------------------------------------------------------------------
 */

struct s2n_bignum_aes_key_st {
  uint64_t rd_key[30];
  int rounds;
};
typedef struct s2n_bignum_aes_key_st s2n_bignum_AES_KEY;

/* AES_XTS_DECRYPT (256-bit) */
/* Inputs in[length], length, key1[244], key2[244], iv[16]; output out[length] */
extern void aes_xts_decrypt(const uint8_t *in, uint8_t *out, size_t length,
        const s2n_bignum_AES_KEY *key1, const s2n_bignum_AES_KEY *key2, const uint8_t iv[16]);

/* AES_XTS_ENCRYPT (256-bit) */
/* Inputs in[length], length, key1[244], key2[244], iv[16]; output out[length] */
extern void aes_xts_encrypt(const uint8_t *in, uint8_t *out, size_t length,
        const s2n_bignum_AES_KEY *key1, const s2n_bignum_AES_KEY *key2, const uint8_t iv[16]);

/* Add, z := x + y */
/* Inputs x[m], y[n]; outputs function return (carry-out) and z[p] */
extern uint64_t bignum_add (uint64_t p, uint64_t *z, uint64_t m, const uint64_t *x, uint64_t n, const uint64_t *y);

/* Add modulo p_25519, z := (x + y) mod p_25519, assuming x and y reduced */
/* Inputs x[4], y[4]; output z[4] */
extern void bignum_add_p25519 (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);

/* Add modulo p_256, z := (x + y) mod p_256, assuming x and y reduced */
/* Inputs x[4], y[4]; output z[4] */
extern void bignum_add_p256 (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);

/* Add modulo p_256k1, z := (x + y) mod p_256k1, assuming x and y reduced */
/* Inputs x[4], y[4]; output z[4] */
extern void bignum_add_p256k1 (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);

/* Add modulo p_384, z := (x + y) mod p_384, assuming x and y reduced */
/* Inputs x[6], y[6]; output z[6] */
extern void bignum_add_p384 (uint64_t z[6], const uint64_t x[6], const uint64_t y[6]);

/* Add modulo p_521, z := (x + y) mod p_521, assuming x and y reduced */
/* Inputs x[9], y[9]; output z[9] */
extern void bignum_add_p521 (uint64_t z[9], const uint64_t x[9], const uint64_t y[9]);

/* Add modulo p_sm2, z := (x + y) mod p_sm2, assuming x and y reduced */
/* Inputs x[4], y[4]; output z[4] */
extern void bignum_add_sm2 (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);

/* Compute "amontification" constant z :== 2^{128k} (congruent mod m) */
/* Input m[k]; output z[k]; temporary buffer t[>=k] */
extern void bignum_amontifier (uint64_t k, uint64_t *z, const uint64_t *m, uint64_t *t);

/* Almost-Montgomery multiply, z :== (x * y / 2^{64k}) (congruent mod m) */
/* Inputs x[k], y[k], m[k]; output z[k] */
extern void bignum_amontmul (uint64_t k, uint64_t *z, const uint64_t *x, const uint64_t *y, const uint64_t *m);

/* Almost-Montgomery reduce, z :== (x' / 2^{64p}) (congruent mod m) */
/* Inputs x[n], m[k], p; output z[k] */
extern void bignum_amontredc (uint64_t k, uint64_t *z, uint64_t n, const uint64_t *x, const uint64_t *m, uint64_t p);

/* Almost-Montgomery square, z :== (x^2 / 2^{64k}) (congruent mod m) */
/* Inputs x[k], m[k]; output z[k] */
extern void bignum_amontsqr (uint64_t k, uint64_t *z, const uint64_t *x, const uint64_t *m);

/* Convert 4-digit (256-bit) bignum to/from big-endian form */
/* Input x[4]; output z[4] */
extern void bignum_bigendian_4 (uint64_t z[4], const uint64_t x[4]);

/* Convert 6-digit (384-bit) bignum to/from big-endian form */
/* Input x[6]; output z[6] */
extern void bignum_bigendian_6 (uint64_t z[6], const uint64_t x[6]);

/* Select bitfield starting at bit n with length l <= 64 */
/* Inputs x[k], n, l; output function return */
extern uint64_t bignum_bitfield (uint64_t k, const uint64_t *x, uint64_t n, uint64_t l);

/* Return size of bignum in bits */
/* Input x[k]; output function return */
extern uint64_t bignum_bitsize (uint64_t k, const uint64_t *x);

/* Divide by a single (nonzero) word, z := x / m and return x mod m */
/* Inputs x[n], m; outputs function return (remainder) and z[k] */
extern uint64_t bignum_cdiv (uint64_t k, uint64_t *z, uint64_t n, const uint64_t *x, uint64_t m);

/* Divide by a single word, z := x / m when known to be exact */
/* Inputs x[n], m; output z[k] */
extern void bignum_cdiv_exact (uint64_t k, uint64_t *z, uint64_t n, const uint64_t *x, uint64_t m);

/* Count leading zero digits (64-bit words) */
/* Input x[k]; output function return */
extern uint64_t bignum_cld (uint64_t k, const uint64_t *x);

/* Count leading zero bits */
/* Input x[k]; output function return */
extern uint64_t bignum_clz (uint64_t k, const uint64_t *x);

/* Multiply-add with single-word multiplier, z := z + c * y */
/* Inputs c, y[n]; outputs function return (carry-out) and z[k] */
extern uint64_t bignum_cmadd (uint64_t k, uint64_t *z, uint64_t c, uint64_t n, const uint64_t *y);

/* Negated multiply-add with single-word multiplier, z := z - c * y */
/* Inputs c, y[n]; outputs function return (negative carry-out) and z[k] */
extern uint64_t bignum_cmnegadd (uint64_t k, uint64_t *z, uint64_t c, uint64_t n, const uint64_t *y);

/* Find modulus of bignum w.r.t. single nonzero word m, returning x mod m */
/* Input x[k], m; output function return */
extern uint64_t bignum_cmod (uint64_t k, const uint64_t *x, uint64_t m);

/* Multiply by a single word, z := c * y */
/* Inputs c, y[n]; outputs function return (carry-out) and z[k] */
extern uint64_t bignum_cmul (uint64_t k, uint64_t *z, uint64_t c, uint64_t n, const uint64_t *y);

/* Multiply by a single word modulo p_25519, z := (c * x) mod p_25519, assuming x reduced */
/* Inputs c, x[4]; output z[4] */
extern void bignum_cmul_p25519 (uint64_t z[4], uint64_t c, const uint64_t x[4]);
extern void bignum_cmul_p25519_alt (uint64_t z[4], uint64_t c, const uint64_t x[4]);

/* Multiply by a single word modulo p_256, z := (c * x) mod p_256, assuming x reduced */
/* Inputs c, x[4]; output z[4] */
extern void bignum_cmul_p256 (uint64_t z[4], uint64_t c, const uint64_t x[4]);
extern void bignum_cmul_p256_alt (uint64_t z[4], uint64_t c, const uint64_t x[4]);

/* Multiply by a single word modulo p_256k1, z := (c * x) mod p_256k1, assuming x reduced */
/* Inputs c, x[4]; output z[4] */
extern void bignum_cmul_p256k1 (uint64_t z[4], uint64_t c, const uint64_t x[4]);
extern void bignum_cmul_p256k1_alt (uint64_t z[4], uint64_t c, const uint64_t x[4]);

/* Multiply by a single word modulo p_384, z := (c * x) mod p_384, assuming x reduced */
/* Inputs c, x[6]; output z[6] */
extern void bignum_cmul_p384 (uint64_t z[6], uint64_t c, const uint64_t x[6]);
extern void bignum_cmul_p384_alt (uint64_t z[6], uint64_t c, const uint64_t x[6]);

/* Multiply by a single word modulo p_521, z := (c * x) mod p_521, assuming x reduced */
/* Inputs c, x[9]; output z[9] */
extern void bignum_cmul_p521 (uint64_t z[9], uint64_t c, const uint64_t x[9]);
extern void bignum_cmul_p521_alt (uint64_t z[9], uint64_t c, const uint64_t x[9]);

/* Multiply by a single word modulo p_sm2, z := (c * x) mod p_sm2, assuming x reduced */
/* Inputs c, x[4]; output z[4] */
extern void bignum_cmul_sm2 (uint64_t z[4], uint64_t c, const uint64_t x[4]);
extern void bignum_cmul_sm2_alt (uint64_t z[4], uint64_t c, const uint64_t x[4]);

/* Test bignums for coprimality, gcd(x,y) = 1 */
/* Inputs x[m], y[n]; output function return; temporary buffer t[>=2*max(m,n)] */
extern uint64_t bignum_coprime (uint64_t m, const uint64_t *x, uint64_t n, const uint64_t *y, uint64_t *t);

/* Copy bignum with zero-extension or truncation, z := x */
/* Input x[n]; output z[k] */
extern void bignum_copy (uint64_t k, uint64_t *z, uint64_t n, const uint64_t *x);

/* Given table: uint64_t[height*width], copy table[idx*width...(idx+1)*width-1] */
/* into z[0..width-1]. */
/* This function is constant-time with respect to the value of `idx`. This is */
/* achieved by reading the whole table and using the bit-masking to get the */
/* `idx`-th row. */
/* Input table[height*width]; output z[width] */
extern void bignum_copy_row_from_table (uint64_t *z, const uint64_t *table, uint64_t height,
        uint64_t width, uint64_t idx);

/* Given table: uint64_t[height*width], copy table[idx*width...(idx+1)*width-1] */
/* into z[0..width-1]. width must be a multiple of 8. */
/* This function is constant-time with respect to the value of `idx`. This is */
/* achieved by reading the whole table and using the bit-masking to get the */
/* `idx`-th row. */
/* Input table[height*width]; output z[width] */
extern void bignum_copy_row_from_table_8n (uint64_t *z, const uint64_t *table,
        uint64_t height, uint64_t width, uint64_t idx);

/* Given table: uint64_t[height*16], copy table[idx*16...(idx+1)*16-1] into z[0..row-1]. */
/* This function is constant-time with respect to the value of `idx`. This is */
/* achieved by reading the whole table and using the bit-masking to get the */
/* `idx`-th row. */
/* Input table[height*16]; output z[16] */
extern void bignum_copy_row_from_table_16 (uint64_t *z, const uint64_t *table,
        uint64_t height, uint64_t idx);

/* Given table: uint64_t[height*32], copy table[idx*32...(idx+1)*32-1] into z[0..row-1]. */
/* This function is constant-time with respect to the value of `idx`. This is */
/* achieved by reading the whole table and using the bit-masking to get the */
/* `idx`-th row. */
/* Input table[height*32]; output z[32] */
extern void bignum_copy_row_from_table_32 (uint64_t *z, const uint64_t *table,
        uint64_t height, uint64_t idx);

/* Count trailing zero digits (64-bit words) */
/* Input x[k]; output function return */
extern uint64_t bignum_ctd (uint64_t k, const uint64_t *x);

/* Count trailing zero bits */
/* Input x[k]; output function return */
extern uint64_t bignum_ctz (uint64_t k, const uint64_t *x);

/* Convert from almost-Montgomery form, z := (x / 2^256) mod p_256 */
/* Input x[4]; output z[4] */
extern void bignum_deamont_p256 (uint64_t z[4], const uint64_t x[4]);
extern void bignum_deamont_p256_alt (uint64_t z[4], const uint64_t x[4]);

/* Convert from almost-Montgomery form, z := (x / 2^256) mod p_256k1 */
/* Input x[4]; output z[4] */
extern void bignum_deamont_p256k1 (uint64_t z[4], const uint64_t x[4]);

/* Convert from almost-Montgomery form, z := (x / 2^384) mod p_384 */
/* Input x[6]; output z[6] */
extern void bignum_deamont_p384 (uint64_t z[6], const uint64_t x[6]);
extern void bignum_deamont_p384_alt (uint64_t z[6], const uint64_t x[6]);

/* Convert from almost-Montgomery form z := (x / 2^576) mod p_521 */
/* Input x[9]; output z[9] */
extern void bignum_deamont_p521 (uint64_t z[9], const uint64_t x[9]);

/* Convert from almost-Montgomery form z := (x / 2^256) mod p_sm2 */
/* Input x[4]; output z[4] */
extern void bignum_deamont_sm2 (uint64_t z[4], const uint64_t x[4]);

/* Convert from (almost-)Montgomery form z := (x / 2^{64k}) mod m */
/* Inputs x[k], m[k]; output z[k] */
extern void bignum_demont (uint64_t k, uint64_t *z, const uint64_t *x, const uint64_t *m);

/* Convert from Montgomery form z := (x / 2^256) mod p_256, assuming x reduced */
/* Input x[4]; output z[4] */
extern void bignum_demont_p256 (uint64_t z[4], const uint64_t x[4]);
extern void bignum_demont_p256_alt (uint64_t z[4], const uint64_t x[4]);

/* Convert from Montgomery form z := (x / 2^256) mod p_256k1, assuming x reduced */
/* Input x[4]; output z[4] */
extern void bignum_demont_p256k1 (uint64_t z[4], const uint64_t x[4]);

/* Convert from Montgomery form z := (x / 2^384) mod p_384, assuming x reduced */
/* Input x[6]; output z[6] */
extern void bignum_demont_p384 (uint64_t z[6], const uint64_t x[6]);
extern void bignum_demont_p384_alt (uint64_t z[6], const uint64_t x[6]);

/* Convert from Montgomery form z := (x / 2^576) mod p_521, assuming x reduced */
/* Input x[9]; output z[9] */
extern void bignum_demont_p521 (uint64_t z[9], const uint64_t x[9]);

/* Convert from Montgomery form z := (x / 2^256) mod p_sm2, assuming x reduced */
/* Input x[4]; output z[4] */
extern void bignum_demont_sm2 (uint64_t z[4], const uint64_t x[4]);

/* Select digit x[n] */
/* Inputs x[k], n; output function return */
extern uint64_t bignum_digit (uint64_t k, const uint64_t *x, uint64_t n);

/* Return size of bignum in digits (64-bit word) */
/* Input x[k]; output function return */
extern uint64_t bignum_digitsize (uint64_t k, const uint64_t *x);

/* Divide bignum by 10: z' := z div 10, returning remainder z mod 10 */
/* Inputs z[k]; outputs function return (remainder) and z[k] */
extern uint64_t bignum_divmod10 (uint64_t k, uint64_t *z);

/* Double modulo p_25519, z := (2 * x) mod p_25519, assuming x reduced */
/* Input x[4]; output z[4] */
extern void bignum_double_p25519 (uint64_t z[4], const uint64_t x[4]);

/* Double modulo p_256, z := (2 * x) mod p_256, assuming x reduced */
/* Input x[4]; output z[4] */
extern void bignum_double_p256 (uint64_t z[4], const uint64_t x[4]);

/* Double modulo p_256k1, z := (2 * x) mod p_256k1, assuming x reduced */
/* Input x[4]; output z[4] */
extern void bignum_double_p256k1 (uint64_t z[4], const uint64_t x[4]);

/* Double modulo p_384, z := (2 * x) mod p_384, assuming x reduced */
/* Input x[6]; output z[6] */
extern void bignum_double_p384 (uint64_t z[6], const uint64_t x[6]);

/* Double modulo p_521, z := (2 * x) mod p_521, assuming x reduced */
/* Input x[9]; output z[9] */
extern void bignum_double_p521 (uint64_t z[9], const uint64_t x[9]);

/* Double modulo p_sm2, z := (2 * x) mod p_sm2, assuming x reduced */
/* Input x[4]; output z[4] */
extern void bignum_double_sm2 (uint64_t z[4], const uint64_t x[4]);

/* Extended Montgomery reduce, returning results in input-output buffer */
/* Inputs z[2*k], m[k], w; outputs function return (extra result bit) and z[2*k] */
extern uint64_t bignum_emontredc (uint64_t k, uint64_t *z, const uint64_t *m, uint64_t w);

/* Extended Montgomery reduce in 8-digit blocks, results in input-output buffer */
/* Inputs z[2*k], m[k], w; outputs function return (extra result bit) and z[2*k] */
extern uint64_t bignum_emontredc_8n (uint64_t k, uint64_t *z, const uint64_t *m, uint64_t w);
/* Inputs z[2*k], m[k], w; outputs function return (extra result bit) and z[2*k] */
/* Temporary buffer m_precalc[12*(k/4-1)] */
extern uint64_t bignum_emontredc_8n_cdiff (uint64_t k, uint64_t *z, const uint64_t *m,
                                          uint64_t w, uint64_t *m_precalc);

/* Test bignums for equality, x = y */
/* Inputs x[m], y[n]; output function return */
extern uint64_t bignum_eq (uint64_t m, const uint64_t *x, uint64_t n, const uint64_t *y);

/* Test bignum for even-ness */
/* Input x[k]; output function return */
extern uint64_t bignum_even (uint64_t k, const uint64_t *x);

/* Convert 4-digit (256-bit) bignum from big-endian bytes */
/* Input x[32] (bytes); output z[4] */
extern void bignum_frombebytes_4 (uint64_t z[4], const uint8_t x[32]);

/* Convert 6-digit (384-bit) bignum from big-endian bytes */
/* Input x[48] (bytes); output z[6] */
extern void bignum_frombebytes_6 (uint64_t z[6], const uint8_t x[48]);

/* Convert 4-digit (256-bit) bignum from little-endian bytes */
/* Input x[32] (bytes); output z[4] */
extern void bignum_fromlebytes_4 (uint64_t z[4], const uint8_t x[32]);

/* Convert 6-digit (384-bit) bignum from little-endian bytes */
/* Input x[48] (bytes); output z[6] */
extern void bignum_fromlebytes_6 (uint64_t z[6], const uint8_t x[48]);

/* Convert little-endian bytes to 9-digit 528-bit bignum */
/* Input x[66] (bytes); output z[9] */
extern void bignum_fromlebytes_p521 (uint64_t z[9],const uint8_t x[66]);

/* Compare bignums, x >= y */
/* Inputs x[m], y[n]; output function return */
extern uint64_t bignum_ge (uint64_t m, const uint64_t *x, uint64_t n, const uint64_t *y);

/* Compare bignums, x > y */
/* Inputs x[m], y[n]; output function return */
extern uint64_t bignum_gt (uint64_t m, const uint64_t *x, uint64_t n, const uint64_t *y);

/* Halve modulo p_256, z := (x / 2) mod p_256, assuming x reduced */
/* Input x[4]; output z[4] */
extern void bignum_half_p256 (uint64_t z[4], const uint64_t x[4]);

/* Halve modulo p_256k1, z := (x / 2) mod p_256k1, assuming x reduced */
/* Input x[4]; output z[4] */
extern void bignum_half_p256k1 (uint64_t z[4], const uint64_t x[4]);

/* Halve modulo p_384, z := (x / 2) mod p_384, assuming x reduced */
/* Input x[6]; output z[6] */
extern void bignum_half_p384 (uint64_t z[6], const uint64_t x[6]);

/* Halve modulo p_521, z := (x / 2) mod p_521, assuming x reduced */
/* Input x[9]; output z[9] */
extern void bignum_half_p521 (uint64_t z[9], const uint64_t x[9]);

/* Halve modulo p_sm2, z := (x / 2) mod p_sm2, assuming x reduced */
/* Input x[4]; output z[4] */
extern void bignum_half_sm2 (uint64_t z[4], const uint64_t x[4]);

/* Modular inverse modulo p_25519 = 2^255 - 19 */
/* Input x[4]; output z[4] */
extern void bignum_inv_p25519(uint64_t z[4],const uint64_t x[4]);

/* Modular inverse modulo p_256 = 2^256 - 2^224 + 2^192 + 2^96 - 1 */
/* Input x[4]; output z[4] */
extern void bignum_inv_p256(uint64_t z[4],const uint64_t x[4]);

/* Modular inverse modulo p_384 = 2^384 - 2^128 - 2^96 + 2^32 - 1 */
/* Input x[6]; output z[6] */
extern void bignum_inv_p384(uint64_t z[6],const uint64_t x[6]);

/* Modular inverse modulo p_521 = 2^521 - 1 */
/* Input x[9]; output z[9] */
extern void bignum_inv_p521(uint64_t z[9],const uint64_t x[9]);

/* Modular inverse modulo p_sm2 = 2^256 - 2^224 - 2^96 + 2^64 - 1 */
/* Input x[4]; output z[4] */
extern void bignum_inv_sm2(uint64_t z[4],const uint64_t x[4]);

/* Inverse square root modulo p_25519 */
/* Input x[4]; output function return (Legendre symbol) and z[4] */
extern int64_t bignum_invsqrt_p25519(uint64_t z[4],const uint64_t x[4]);
extern int64_t bignum_invsqrt_p25519_alt(uint64_t z[4],const uint64_t x[4]);

/* Test bignum for zero-ness, x = 0 */
/* Input x[k]; output function return */
extern uint64_t bignum_iszero (uint64_t k, const uint64_t *x);

/* Multiply z := x * y */
/* Inputs x[16], y[16]; output z[32]; temporary buffer t[>=32] */
extern void bignum_kmul_16_32 (uint64_t z[32], const uint64_t x[16], const uint64_t y[16], uint64_t t[32]);

/* Multiply z := x * y */
/* Inputs x[32], y[32]; output z[64]; temporary buffer t[>=96] */
extern void bignum_kmul_32_64 (uint64_t z[64], const uint64_t x[32], const uint64_t y[32], uint64_t t[96]);

/* Square, z := x^2 */
/* Input x[16]; output z[32]; temporary buffer t[>=24] */
extern void bignum_ksqr_16_32 (uint64_t z[32], const uint64_t x[16], uint64_t t[24]);

/* Square, z := x^2 */
/* Input x[32]; output z[64]; temporary buffer t[>=72] */
extern void bignum_ksqr_32_64 (uint64_t z[64], const uint64_t x[32], uint64_t t[72]);

/* Compare bignums, x <= y */
/* Inputs x[m], y[n]; output function return */
extern uint64_t bignum_le (uint64_t m, const uint64_t *x, uint64_t n, const uint64_t *y);

/* Convert 4-digit (256-bit) bignum to/from little-endian form */
/* Input x[4]; output z[4] */
extern void bignum_littleendian_4 (uint64_t z[4], const uint64_t x[4]);

/* Convert 6-digit (384-bit) bignum to/from little-endian form */
/* Input x[6]; output z[6] */
extern void bignum_littleendian_6 (uint64_t z[6], const uint64_t x[6]);

/* Compare bignums, x < y */
/* Inputs x[m], y[n]; output function return */
extern uint64_t bignum_lt (uint64_t m, const uint64_t *x, uint64_t n, const uint64_t *y);

/* Multiply-add, z := z + x * y */
/* Inputs x[m], y[n]; outputs function return (carry-out) and z[k] */
extern uint64_t bignum_madd (uint64_t k, uint64_t *z, uint64_t m, const uint64_t *x, uint64_t n, const uint64_t *y);

/* Multiply-add modulo the order of the curve25519/edwards25519 basepoint */
/* Inputs x[4], y[4], c[4]; output z[4] */
extern void bignum_madd_n25519 (uint64_t z[4], const uint64_t x[4], const uint64_t y[4], const uint64_t c[4]);
extern void bignum_madd_n25519_alt (uint64_t z[4], const uint64_t x[4], const uint64_t y[4], const uint64_t c[4]);

/* Reduce modulo group order, z := x mod m_25519 */
/* Input x[k]; output z[4] */
extern void bignum_mod_m25519 (uint64_t z[4], uint64_t k, const uint64_t *x);

/* Reduce modulo group order, z := x mod m_25519 */
/* Input x[4]; output z[4] */
extern void bignum_mod_m25519_4 (uint64_t z[4], const uint64_t x[4]);

/* Reduce modulo basepoint order, z := x mod n_25519 */
/* Input x[k]; output z[4] */
extern void bignum_mod_n25519 (uint64_t z[4], uint64_t k, const uint64_t *x);

/* Reduce modulo basepoint order, z := x mod n_25519 */
/* Input x[4]; output z[4] */
extern void bignum_mod_n25519_4 (uint64_t z[4], const uint64_t x[4]);

/* Reduce modulo group order, z := x mod n_256 */
/* Input x[k]; output z[4] */
extern void bignum_mod_n256 (uint64_t z[4], uint64_t k, const uint64_t *x);
extern void bignum_mod_n256_alt (uint64_t z[4], uint64_t k, const uint64_t *x);

/* Reduce modulo group order, z := x mod n_256 */
/* Input x[4]; output z[4] */
extern void bignum_mod_n256_4 (uint64_t z[4], const uint64_t x[4]);

/* Reduce modulo group order, z := x mod n_256k1 */
/* Input x[k]; output z[4] */
extern void bignum_mod_n256k1 (uint64_t z[4], uint64_t k, const uint64_t *x);

/* Reduce modulo group order, z := x mod n_256k1 */
/* Input x[4]; output z[4] */
extern void bignum_mod_n256k1_4 (uint64_t z[4], const uint64_t x[4]);

/* Reduce modulo group order, z := x mod n_384 */
/* Input x[k]; output z[6] */
extern void bignum_mod_n384 (uint64_t z[6], uint64_t k, const uint64_t *x);
extern void bignum_mod_n384_alt (uint64_t z[6], uint64_t k, const uint64_t *x);

/* Reduce modulo group order, z := x mod n_384 */
/* Input x[6]; output z[6] */
extern void bignum_mod_n384_6 (uint64_t z[6], const uint64_t x[6]);

/* Reduce modulo group order, z := x mod n_521 */
/* Input x[9]; output z[9] */
extern void bignum_mod_n521_9 (uint64_t z[9], const uint64_t x[9]);
extern void bignum_mod_n521_9_alt (uint64_t z[9], const uint64_t x[9]);

/* Reduce modulo group order, z := x mod n_sm2 */
/* Input x[k]; output z[4] */
extern void bignum_mod_nsm2 (uint64_t z[4], uint64_t k, const uint64_t *x);
extern void bignum_mod_nsm2_alt (uint64_t z[4], uint64_t k, const uint64_t *x);

/* Reduce modulo group order, z := x mod n_sm2 */
/* Input x[4]; output z[4] */
extern void bignum_mod_nsm2_4 (uint64_t z[4], const uint64_t x[4]);

/* Reduce modulo field characteristic, z := x mod p_25519 */
/* Input x[4]; output z[4] */
extern void bignum_mod_p25519_4 (uint64_t z[4], const uint64_t x[4]);

/* Reduce modulo field characteristic, z := x mod p_256 */
/* Input x[k]; output z[4] */
extern void bignum_mod_p256 (uint64_t z[4], uint64_t k, const uint64_t *x);
extern void bignum_mod_p256_alt (uint64_t z[4], uint64_t k, const uint64_t *x);

/* Reduce modulo field characteristic, z := x mod p_256 */
/* Input x[4]; output z[4] */
extern void bignum_mod_p256_4 (uint64_t z[4], const uint64_t x[4]);

/* Reduce modulo field characteristic, z := x mod p_256k1 */
/* Input x[k]; output z[4] */
extern void bignum_mod_p256k1 (uint64_t z[4], uint64_t k, const uint64_t *x);

/* Reduce modulo field characteristic, z := x mod p_256k1 */
/* Input x[4]; output z[4] */
extern void bignum_mod_p256k1_4 (uint64_t z[4], const uint64_t x[4]);

/* Reduce modulo field characteristic, z := x mod p_384 */
/* Input x[k]; output z[6] */
extern void bignum_mod_p384 (uint64_t z[6], uint64_t k, const uint64_t *x);
extern void bignum_mod_p384_alt (uint64_t z[6], uint64_t k, const uint64_t *x);

/* Reduce modulo field characteristic, z := x mod p_384 */
/* Input x[6]; output z[6] */
extern void bignum_mod_p384_6 (uint64_t z[6], const uint64_t x[6]);

/* Reduce modulo field characteristic, z := x mod p_521 */
/* Input x[9]; output z[9] */
extern void bignum_mod_p521_9 (uint64_t z[9], const uint64_t x[9]);

/* Reduce modulo field characteristic, z := x mod p_sm2 */
/* Input x[k]; output z[4] */
extern void bignum_mod_sm2 (uint64_t z[4], uint64_t k, const uint64_t *x);

/* Reduce modulo field characteristic, z := x mod p_sm2 */
/* Input x[4]; output z[4] */
extern void bignum_mod_sm2_4 (uint64_t z[4], const uint64_t x[4]);

/* Add modulo m, z := (x + y) mod m, assuming x and y reduced */
/* Inputs x[k], y[k], m[k]; output z[k] */
extern void bignum_modadd (uint64_t k, uint64_t *z, const uint64_t *x, const uint64_t *y, const uint64_t *m);

/* Double modulo m, z := (2 * x) mod m, assuming x reduced */
/* Inputs x[k], m[k]; output z[k] */
extern void bignum_moddouble (uint64_t k, uint64_t *z, const uint64_t *x, const uint64_t *m);

/* Modular exponentiation for arbitrary odd modulus, z := (a^p) mod m */
/* Inputs a[k], p[k], m[k]; output z[k]; temporary buffer t[>=3*k] */
extern void bignum_modexp(uint64_t k,uint64_t *z, const uint64_t *a,const uint64_t *p,const uint64_t *m,uint64_t *t);

/* Compute "modification" constant z := 2^{64k} mod m */
/* Input m[k]; output z[k]; temporary buffer t[>=k] */
extern void bignum_modifier (uint64_t k, uint64_t *z, const uint64_t *m, uint64_t *t);

/* Invert modulo m, z = (1/a) mod b, assuming b is an odd number > 1, a coprime to b */
/* Inputs a[k], b[k]; output z[k]; temporary buffer t[>=3*k] */
extern void bignum_modinv (uint64_t k, uint64_t *z, const uint64_t *a, const uint64_t *b, uint64_t *t);

/* Optionally negate modulo m, z := (-x) mod m (if p nonzero) or z := x (if p zero), assuming x reduced */
/* Inputs p, x[k], m[k]; output z[k] */
extern void bignum_modoptneg (uint64_t k, uint64_t *z, uint64_t p, const uint64_t *x, const uint64_t *m);

/* Subtract modulo m, z := (x - y) mod m, assuming x and y reduced */
/* Inputs x[k], y[k], m[k]; output z[k] */
extern void bignum_modsub (uint64_t k, uint64_t *z, const uint64_t *x, const uint64_t *y, const uint64_t *m);

/* Compute "montification" constant z := 2^{128k} mod m */
/* Input m[k]; output z[k]; temporary buffer t[>=k] */
extern void bignum_montifier (uint64_t k, uint64_t *z, const uint64_t *m, uint64_t *t);

/* Montgomery inverse modulo p_256 = 2^256 - 2^224 + 2^192 + 2^96 - 1 */
/* Input x[4]; output z[4] */
extern void bignum_montinv_p256(uint64_t z[4],const uint64_t x[4]);

/* Montgomery inverse modulo p_384 = 2^384 - 2^128 - 2^96 + 2^32 - 1 */
/* Input x[6]; output z[6] */
extern void bignum_montinv_p384(uint64_t z[6],const uint64_t x[6]);

/* Montgomery inverse modulo p_sm2 = 2^256 - 2^224 - 2^96 + 2^64 - 1 */
/* Input x[4]; output z[4] */
extern void bignum_montinv_sm2(uint64_t z[4],const uint64_t x[4]);

/* Montgomery multiply, z := (x * y / 2^{64k}) mod m */
/* Inputs x[k], y[k], m[k]; output z[k] */
extern void bignum_montmul (uint64_t k, uint64_t *z, const uint64_t *x, const uint64_t *y, const uint64_t *m);

/* Montgomery multiply, z := (x * y / 2^256) mod p_256 */
/* Inputs x[4], y[4]; output z[4] */
extern void bignum_montmul_p256 (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);
extern void bignum_montmul_p256_alt (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);

/* Montgomery multiply, z := (x * y / 2^256) mod p_256k1 */
/* Inputs x[4], y[4]; output z[4] */
extern void bignum_montmul_p256k1 (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);
extern void bignum_montmul_p256k1_alt (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);

/* Montgomery multiply, z := (x * y / 2^384) mod p_384 */
/* Inputs x[6], y[6]; output z[6] */
extern void bignum_montmul_p384 (uint64_t z[6], const uint64_t x[6], const uint64_t y[6]);
extern void bignum_montmul_p384_alt (uint64_t z[6], const uint64_t x[6], const uint64_t y[6]);

/* Montgomery multiply, z := (x * y / 2^576) mod p_521 */
/* Inputs x[9], y[9]; output z[9] */
extern void bignum_montmul_p521 (uint64_t z[9], const uint64_t x[9], const uint64_t y[9]);
extern void bignum_montmul_p521_alt (uint64_t z[9], const uint64_t x[9], const uint64_t y[9]);

/* Montgomery multiply, z := (x * y / 2^256) mod p_sm2 */
/* Inputs x[4], y[4]; output z[4] */
extern void bignum_montmul_sm2 (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);
extern void bignum_montmul_sm2_alt (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);

/* Montgomery reduce, z := (x' / 2^{64p}) MOD m */
/* Inputs x[n], m[k], p; output z[k] */
extern void bignum_montredc (uint64_t k, uint64_t *z, uint64_t n, const uint64_t *x, const uint64_t *m, uint64_t p);

/* Montgomery square, z := (x^2 / 2^{64k}) mod m */
/* Inputs x[k], m[k]; output z[k] */
extern void bignum_montsqr (uint64_t k, uint64_t *z, const uint64_t *x, const uint64_t *m);

/* Montgomery square, z := (x^2 / 2^256) mod p_256 */
/* Input x[4]; output z[4] */
extern void bignum_montsqr_p256 (uint64_t z[4], const uint64_t x[4]);
extern void bignum_montsqr_p256_alt (uint64_t z[4], const uint64_t x[4]);

/* Montgomery square, z := (x^2 / 2^256) mod p_256k1 */
/* Input x[4]; output z[4] */
extern void bignum_montsqr_p256k1 (uint64_t z[4], const uint64_t x[4]);
extern void bignum_montsqr_p256k1_alt (uint64_t z[4], const uint64_t x[4]);

/* Montgomery square, z := (x^2 / 2^384) mod p_384 */
/* Input x[6]; output z[6] */
extern void bignum_montsqr_p384 (uint64_t z[6], const uint64_t x[6]);
extern void bignum_montsqr_p384_alt (uint64_t z[6], const uint64_t x[6]);

/* Montgomery square, z := (x^2 / 2^576) mod p_521 */
/* Input x[9]; output z[9] */
extern void bignum_montsqr_p521 (uint64_t z[9], const uint64_t x[9]);
extern void bignum_montsqr_p521_alt (uint64_t z[9], const uint64_t x[9]);

/* Montgomery square, z := (x^2 / 2^256) mod p_sm2 */
/* Input x[4]; output z[4] */
extern void bignum_montsqr_sm2 (uint64_t z[4], const uint64_t x[4]);
extern void bignum_montsqr_sm2_alt (uint64_t z[4], const uint64_t x[4]);

/* Multiply z := x * y */
/* Inputs x[m], y[n]; output z[k] */
extern void bignum_mul (uint64_t k, uint64_t *z, uint64_t m, const uint64_t *x, uint64_t n, const uint64_t *y);

/* Multiply z := x * y */
/* Inputs x[4], y[4]; output z[8] */
extern void bignum_mul_4_8 (uint64_t z[8], const uint64_t x[4], const uint64_t y[4]);
extern void bignum_mul_4_8_alt (uint64_t z[8], const uint64_t x[4], const uint64_t y[4]);

/* Multiply z := x * y */
/* Inputs x[6], y[6]; output z[12] */
extern void bignum_mul_6_12 (uint64_t z[12], const uint64_t x[6], const uint64_t y[6]);
extern void bignum_mul_6_12_alt (uint64_t z[12], const uint64_t x[6], const uint64_t y[6]);

/* Multiply z := x * y */
/* Inputs x[8], y[8]; output z[16] */
extern void bignum_mul_8_16 (uint64_t z[16], const uint64_t x[8], const uint64_t y[8]);
extern void bignum_mul_8_16_alt (uint64_t z[16], const uint64_t x[8], const uint64_t y[8]);

/* Multiply modulo p_25519, z := (x * y) mod p_25519 */
/* Inputs x[4], y[4]; output z[4] */
extern void bignum_mul_p25519 (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);
extern void bignum_mul_p25519_alt (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);

/* Multiply modulo p_256k1, z := (x * y) mod p_256k1 */
/* Inputs x[4], y[4]; output z[4] */
extern void bignum_mul_p256k1 (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);
extern void bignum_mul_p256k1_alt (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);

/* Multiply modulo p_521, z := (x * y) mod p_521, assuming x and y reduced */
/* Inputs x[9], y[9]; output z[9] */
extern void bignum_mul_p521 (uint64_t z[9], const uint64_t x[9], const uint64_t y[9]);
extern void bignum_mul_p521_alt (uint64_t z[9], const uint64_t x[9], const uint64_t y[9]);

/* Multiply bignum by 10 and add word: z := 10 * z + d */
/* Inputs z[k], d; outputs function return (carry) and z[k] */
extern uint64_t bignum_muladd10 (uint64_t k, uint64_t *z, uint64_t d);

/* Multiplex/select z := x (if p nonzero) or z := y (if p zero) */
/* Inputs p, x[k], y[k]; output z[k] */
extern void bignum_mux (uint64_t p, uint64_t k, uint64_t *z, const uint64_t *x, const uint64_t *y);

/* 256-bit multiplex/select z := x (if p nonzero) or z := y (if p zero) */
/* Inputs p, x[4], y[4]; output z[4] */
extern void bignum_mux_4 (uint64_t p, uint64_t z[4],const uint64_t x[4], const uint64_t y[4]);

/* 384-bit multiplex/select z := x (if p nonzero) or z := y (if p zero) */
/* Inputs p, x[6], y[6]; output z[6] */
extern void bignum_mux_6 (uint64_t p, uint64_t z[6],const uint64_t x[6], const uint64_t y[6]);

/* Select element from 16-element table, z := xs[k*i] */
/* Inputs xs[16*k], i; output z[k] */
extern void bignum_mux16 (uint64_t k, uint64_t *z, const uint64_t *xs, uint64_t i);

/* Negate modulo p_25519, z := (-x) mod p_25519, assuming x reduced */
/* Input x[4]; output z[4] */
extern void bignum_neg_p25519 (uint64_t z[4], const uint64_t x[4]);

/* Negate modulo p_256, z := (-x) mod p_256, assuming x reduced */
/* Input x[4]; output z[4] */
extern void bignum_neg_p256 (uint64_t z[4], const uint64_t x[4]);

/* Negate modulo p_256k1, z := (-x) mod p_256k1, assuming x reduced */
/* Input x[4]; output z[4] */
extern void bignum_neg_p256k1 (uint64_t z[4], const uint64_t x[4]);

/* Negate modulo p_384, z := (-x) mod p_384, assuming x reduced */
/* Input x[6]; output z[6] */
extern void bignum_neg_p384 (uint64_t z[6], const uint64_t x[6]);

/* Negate modulo p_521, z := (-x) mod p_521, assuming x reduced */
/* Input x[9]; output z[9] */
extern void bignum_neg_p521 (uint64_t z[9], const uint64_t x[9]);

/* Negate modulo p_sm2, z := (-x) mod p_sm2, assuming x reduced */
/* Input x[4]; output z[4] */
extern void bignum_neg_sm2 (uint64_t z[4], const uint64_t x[4]);

/* Negated modular inverse, z := (-1/x) mod 2^{64k} */
/* Input x[k]; output z[k] */
extern void bignum_negmodinv (uint64_t k, uint64_t *z, const uint64_t *x);

/* Test bignum for nonzero-ness x =/= 0 */
/* Input x[k]; output function return */
extern uint64_t bignum_nonzero (uint64_t k, const uint64_t *x);

/* Test 256-bit bignum for nonzero-ness x =/= 0 */
/* Input x[4]; output function return */
extern uint64_t bignum_nonzero_4(const uint64_t x[4]);

/* Test 384-bit bignum for nonzero-ness x =/= 0 */
/* Input x[6]; output function return */
extern uint64_t bignum_nonzero_6(const uint64_t x[6]);

/* Normalize bignum in-place by shifting left till top bit is 1 */
/* Input z[k]; outputs function return (bits shifted left) and z[k] */
extern uint64_t bignum_normalize (uint64_t k, uint64_t *z);

/* Test bignum for odd-ness */
/* Input x[k]; output function return */
extern uint64_t bignum_odd (uint64_t k, const uint64_t *x);

/* Convert single digit to bignum, z := n */
/* Input n; output z[k] */
extern void bignum_of_word (uint64_t k, uint64_t *z, uint64_t n);

/* Optionally add, z := x + y (if p nonzero) or z := x (if p zero) */
/* Inputs x[k], p, y[k]; outputs function return (carry-out) and z[k] */
extern uint64_t bignum_optadd (uint64_t k, uint64_t *z, const uint64_t *x, uint64_t p, const uint64_t *y);

/* Optionally negate, z := -x (if p nonzero) or z := x (if p zero) */
/* Inputs p, x[k]; outputs function return (nonzero input) and z[k] */
extern uint64_t bignum_optneg (uint64_t k, uint64_t *z, uint64_t p, const uint64_t *x);

/* Optionally negate modulo p_25519, z := (-x) mod p_25519 (if p nonzero) or z := x (if p zero), assuming x reduced */
/* Inputs p, x[4]; output z[4] */
extern void bignum_optneg_p25519 (uint64_t z[4], uint64_t p, const uint64_t x[4]);

/* Optionally negate modulo p_256, z := (-x) mod p_256 (if p nonzero) or z := x (if p zero), assuming x reduced */
/* Inputs p, x[4]; output z[4] */
extern void bignum_optneg_p256 (uint64_t z[4], uint64_t p, const uint64_t x[4]);

/* Optionally negate modulo p_256k1, z := (-x) mod p_256k1 (if p nonzero) or z := x (if p zero), assuming x reduced */
/* Inputs p, x[4]; output z[4] */
extern void bignum_optneg_p256k1 (uint64_t z[4], uint64_t p, const uint64_t x[4]);

/* Optionally negate modulo p_384, z := (-x) mod p_384 (if p nonzero) or z := x (if p zero), assuming x reduced */
/* Inputs p, x[6]; output z[6] */
extern void bignum_optneg_p384 (uint64_t z[6], uint64_t p, const uint64_t x[6]);

/* Optionally negate modulo p_521, z := (-x) mod p_521 (if p nonzero) or z := x (if p zero), assuming x reduced */
/* Inputs p, x[9]; output z[9] */
extern void bignum_optneg_p521 (uint64_t z[9], uint64_t p, const uint64_t x[9]);

/* Optionally negate modulo p_sm2, z := (-x) mod p_sm2 (if p nonzero) or z := x (if p zero), assuming x reduced */
/* Inputs p, x[4]; output z[4] */
extern void bignum_optneg_sm2 (uint64_t z[4], uint64_t p, const uint64_t x[4]);

/* Optionally subtract, z := x - y (if p nonzero) or z := x (if p zero) */
/* Inputs x[k], p, y[k]; outputs function return (carry-out) and z[k] */
extern uint64_t bignum_optsub (uint64_t k, uint64_t *z, const uint64_t *x, uint64_t p, const uint64_t *y);

/* Optionally subtract or add, z := x + sgn(p) * y interpreting p as signed */
/* Inputs x[k], p, y[k]; outputs function return (carry-out) and z[k] */
extern uint64_t bignum_optsubadd (uint64_t k, uint64_t *z, const uint64_t *x, uint64_t p, const uint64_t *y);

/* Return bignum of power of 2, z := 2^n */
/* Input n; output z[k] */
extern void bignum_pow2 (uint64_t k, uint64_t *z, uint64_t n);

/* Shift bignum left by c < 64 bits z := x * 2^c */
/* Inputs x[n], c; outputs function return (carry-out) and z[k] */
extern uint64_t bignum_shl_small (uint64_t k, uint64_t *z, uint64_t n, const uint64_t *x, uint64_t c);

/* Shift bignum right by c < 64 bits z := floor(x / 2^c) */
/* Inputs x[n], c; outputs function return (bits shifted out) and z[k] */
extern uint64_t bignum_shr_small (uint64_t k, uint64_t *z, uint64_t n, const uint64_t *x, uint64_t c);

/* Square, z := x^2 */
/* Input x[n]; output z[k] */
extern void bignum_sqr (uint64_t k, uint64_t *z, uint64_t n, const uint64_t *x);

/* Square, z := x^2 */
/* Input x[4]; output z[8] */
extern void bignum_sqr_4_8 (uint64_t z[8], const uint64_t x[4]);
extern void bignum_sqr_4_8_alt (uint64_t z[8], const uint64_t x[4]);

/* Square, z := x^2 */
/* Input x[6]; output z[12] */
extern void bignum_sqr_6_12 (uint64_t z[12], const uint64_t x[6]);
extern void bignum_sqr_6_12_alt (uint64_t z[12], const uint64_t x[6]);

/* Square, z := x^2 */
/* Input x[8]; output z[16] */
extern void bignum_sqr_8_16 (uint64_t z[16], const uint64_t x[8]);
extern void bignum_sqr_8_16_alt (uint64_t z[16], const uint64_t x[8]);

/* Square modulo p_25519, z := (x^2) mod p_25519 */
/* Input x[4]; output z[4] */
extern void bignum_sqr_p25519 (uint64_t z[4], const uint64_t x[4]);
extern void bignum_sqr_p25519_alt (uint64_t z[4], const uint64_t x[4]);

/* Square modulo p_256k1, z := (x^2) mod p_256k1 */
/* Input x[4]; output z[4] */
extern void bignum_sqr_p256k1 (uint64_t z[4], const uint64_t x[4]);
extern void bignum_sqr_p256k1_alt (uint64_t z[4], const uint64_t x[4]);

/* Square modulo p_521, z := (x^2) mod p_521, assuming x reduced */
/* Input x[9]; output z[9] */
extern void bignum_sqr_p521 (uint64_t z[9], const uint64_t x[9]);
extern void bignum_sqr_p521_alt (uint64_t z[9], const uint64_t x[9]);

/* Square root modulo p_25519 */
/* Input x[4]; output function return (Legendre symbol) and z[4] */
extern int64_t bignum_sqrt_p25519(uint64_t z[4],const uint64_t x[4]);
extern int64_t bignum_sqrt_p25519_alt(uint64_t z[4],const uint64_t x[4]);

/* Subtract, z := x - y */
/* Inputs x[m], y[n]; outputs function return (carry-out) and z[p] */
extern uint64_t bignum_sub (uint64_t p, uint64_t *z, uint64_t m, const uint64_t *x, uint64_t n, const uint64_t *y);

/* Subtract modulo p_25519, z := (x - y) mod p_25519, assuming x and y reduced */
/* Inputs x[4], y[4]; output z[4] */
extern void bignum_sub_p25519 (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);

/* Subtract modulo p_256, z := (x - y) mod p_256, assuming x and y reduced */
/* Inputs x[4], y[4]; output z[4] */
extern void bignum_sub_p256 (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);

/* Subtract modulo p_256k1, z := (x - y) mod p_256k1, assuming x and y reduced */
/* Inputs x[4], y[4]; output z[4] */
extern void bignum_sub_p256k1 (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);

/* Subtract modulo p_384, z := (x - y) mod p_384, assuming x and y reduced */
/* Inputs x[6], y[6]; output z[6] */
extern void bignum_sub_p384 (uint64_t z[6], const uint64_t x[6], const uint64_t y[6]);

/* Subtract modulo p_521, z := (x - y) mod p_521, assuming x and y reduced */
/* Inputs x[9], y[9]; output z[9] */
extern void bignum_sub_p521 (uint64_t z[9], const uint64_t x[9], const uint64_t y[9]);

/* Subtract modulo p_sm2, z := (x - y) mod p_sm2, assuming x and y reduced */
/* Inputs x[4], y[4]; output z[4] */
extern void bignum_sub_sm2 (uint64_t z[4], const uint64_t x[4], const uint64_t y[4]);

/* Convert 4-digit (256-bit) bignum to big-endian bytes */
/* Input x[4]; output z[32] (bytes) */
extern void bignum_tobebytes_4 (uint8_t z[32], const uint64_t x[4]);

/* Convert 6-digit (384-bit) bignum to big-endian bytes */
/* Input x[6]; output z[48] (bytes) */
extern void bignum_tobebytes_6 (uint8_t z[48], const uint64_t x[6]);

/* Convert 4-digit (256-bit) bignum to little-endian bytes */
/* Input x[4]; output z[32] (bytes) */
extern void bignum_tolebytes_4 (uint8_t z[32], const uint64_t x[4]);

/* Convert 6-digit (384-bit) bignum to little-endian bytes */
/* Input x[6]; output z[48] (bytes) */
extern void bignum_tolebytes_6 (uint8_t z[48], const uint64_t x[6]);

/* Convert 9-digit 528-bit bignum to little-endian bytes */
/* Input x[9]; output z[66] (bytes) */
extern void bignum_tolebytes_p521 (uint8_t z[66], const uint64_t x[9]);

/* Convert to Montgomery form z := (2^256 * x) mod p_256 */
/* Input x[4]; output z[4] */
extern void bignum_tomont_p256 (uint64_t z[4], const uint64_t x[4]);
extern void bignum_tomont_p256_alt (uint64_t z[4], const uint64_t x[4]);

/* Convert to Montgomery form z := (2^256 * x) mod p_256k1 */
/* Input x[4]; output z[4] */
extern void bignum_tomont_p256k1 (uint64_t z[4], const uint64_t x[4]);
extern void bignum_tomont_p256k1_alt (uint64_t z[4], const uint64_t x[4]);

/* Convert to Montgomery form z := (2^384 * x) mod p_384 */
/* Input x[6]; output z[6] */
extern void bignum_tomont_p384 (uint64_t z[6], const uint64_t x[6]);
extern void bignum_tomont_p384_alt (uint64_t z[6], const uint64_t x[6]);

/* Convert to Montgomery form z := (2^576 * x) mod p_521 */
/* Input x[9]; output z[9] */
extern void bignum_tomont_p521 (uint64_t z[9], const uint64_t x[9]);

/* Convert to Montgomery form z := (2^256 * x) mod p_sm2 */
/* Input x[4]; output z[4] */
extern void bignum_tomont_sm2 (uint64_t z[4], const uint64_t x[4]);

/* Triple modulo p_256, z := (3 * x) mod p_256 */
/* Input x[4]; output z[4] */
extern void bignum_triple_p256 (uint64_t z[4], const uint64_t x[4]);
extern void bignum_triple_p256_alt (uint64_t z[4], const uint64_t x[4]);

/* Triple modulo p_256k1, z := (3 * x) mod p_256k1 */
/* Input x[4]; output z[4] */
extern void bignum_triple_p256k1 (uint64_t z[4], const uint64_t x[4]);
extern void bignum_triple_p256k1_alt (uint64_t z[4], const uint64_t x[4]);

/* Triple modulo p_384, z := (3 * x) mod p_384 */
/* Input x[6]; output z[6] */
extern void bignum_triple_p384 (uint64_t z[6], const uint64_t x[6]);
extern void bignum_triple_p384_alt (uint64_t z[6], const uint64_t x[6]);

/* Triple modulo p_521, z := (3 * x) mod p_521, assuming x reduced */
/* Input x[9]; output z[9] */
extern void bignum_triple_p521 (uint64_t z[9], const uint64_t x[9]);
extern void bignum_triple_p521_alt (uint64_t z[9], const uint64_t x[9]);

/* Triple modulo p_sm2, z := (3 * x) mod p_sm2 */
/* Input x[4]; output z[4] */
extern void bignum_triple_sm2 (uint64_t z[4], const uint64_t x[4]);
extern void bignum_triple_sm2_alt (uint64_t z[4], const uint64_t x[4]);

/* Montgomery ladder step for curve25519 */
/* Inputs point[8], pp[16], b; output rr[16] */
extern void curve25519_ladderstep(uint64_t rr[16],const uint64_t point[8],const uint64_t pp[16],uint64_t b);
extern void curve25519_ladderstep_alt(uint64_t rr[16],const uint64_t point[8],const uint64_t pp[16],uint64_t b);

/* Projective scalar multiplication, x coordinate only, for curve25519 */
/* Inputs scalar[4], point[4]; output res[8] */
extern void curve25519_pxscalarmul(uint64_t res[8],const uint64_t scalar[4],const uint64_t point[4]);
extern void curve25519_pxscalarmul_alt(uint64_t res[8],const uint64_t scalar[4],const uint64_t point[4]);

/* x25519 function for curve25519 */
/* Inputs scalar[4], point[4]; output res[4] */
extern void curve25519_x25519(uint64_t res[4],const uint64_t scalar[4],const uint64_t point[4]);
extern void curve25519_x25519_alt(uint64_t res[4],const uint64_t scalar[4],const uint64_t point[4]);

/* x25519 function for curve25519 (byte array arguments) */
/* Inputs scalar[32] (bytes), point[32] (bytes); output res[32] (bytes) */
extern void curve25519_x25519_byte(uint8_t res[32],const uint8_t scalar[32],const uint8_t point[32]);
extern void curve25519_x25519_byte_alt(uint8_t res[32],const uint8_t scalar[32],const uint8_t point[32]);

/* x25519 function for curve25519 on base element 9 */
/* Input scalar[4]; output res[4] */
extern void curve25519_x25519base(uint64_t res[4],const uint64_t scalar[4]);
extern void curve25519_x25519base_alt(uint64_t res[4],const uint64_t scalar[4]);

/* x25519 function for curve25519 on base element 9 (byte array arguments) */
/* Input scalar[32] (bytes); output res[32] (bytes) */
extern void curve25519_x25519base_byte(uint8_t res[32],const uint8_t scalar[32]);
extern void curve25519_x25519base_byte_alt(uint8_t res[32],const uint8_t scalar[32]);

/* Decode compressed 256-bit form of edwards25519 point */
/* Input c[32] (bytes); output function return and z[8] */
extern uint64_t edwards25519_decode(uint64_t z[8], const uint8_t c[32]);
extern uint64_t edwards25519_decode_alt(uint64_t z[8], const uint8_t c[32]);

/* Encode edwards25519 point into compressed form as 256-bit number */
/* Input p[8]; output z[32] (bytes) */
extern void edwards25519_encode(uint8_t z[32], const uint64_t p[8]);

/* Extended projective addition for edwards25519 */
/* Inputs p1[16], p2[16]; output p3[16] */
extern void edwards25519_epadd(uint64_t p3[16],const uint64_t p1[16],const uint64_t p2[16]);
extern void edwards25519_epadd_alt(uint64_t p3[16],const uint64_t p1[16],const uint64_t p2[16]);

/* Extended projective doubling for edwards25519 */
/* Inputs p1[12]; output p3[16] */
extern void edwards25519_epdouble(uint64_t p3[16],const uint64_t p1[12]);
extern void edwards25519_epdouble_alt(uint64_t p3[16],const uint64_t p1[12]);

/* Projective doubling for edwards25519 */
/* Inputs p1[12]; output p3[12] */
extern void edwards25519_pdouble(uint64_t p3[12],const uint64_t p1[12]);
extern void edwards25519_pdouble_alt(uint64_t p3[12],const uint64_t p1[12]);

/* Extended projective + precomputed mixed addition for edwards25519 */
/* Inputs p1[16], p2[12]; output p3[16] */
extern void edwards25519_pepadd(uint64_t p3[16],const uint64_t p1[16],const uint64_t p2[12]);
extern void edwards25519_pepadd_alt(uint64_t p3[16],const uint64_t p1[16],const uint64_t p2[12]);

/* Scalar multiplication by standard basepoint for edwards25519 (Ed25519) */
/* Input scalar[4]; output res[8] */
extern void edwards25519_scalarmulbase(uint64_t res[8],const uint64_t scalar[4]);
extern void edwards25519_scalarmulbase_alt(uint64_t res[8],const uint64_t scalar[4]);

/* Double scalar multiplication for edwards25519, fresh and base point */
/* Input scalar[4], point[8], bscalar[4]; output res[8] */
extern void edwards25519_scalarmuldouble(uint64_t res[8],const uint64_t scalar[4], const uint64_t point[8],const uint64_t bscalar[4]);
extern void edwards25519_scalarmuldouble_alt(uint64_t res[8],const uint64_t scalar[4], const uint64_t point[8],const uint64_t bscalar[4]);

/* Forward number-theoretic transform for ML-DSA */
/* Input a[256], z_012345[144], z_67[384] (signed 32-bit words); output a[256] (signed 32-bit words) */
extern void mldsa_ntt_arm(int32_t a[256], const int32_t z_012345[144], const int32_t z_67[384]);

/* Inverse number-theoretic transform for ML-DSA */
/* Input a[256], z_78[384], z_123456[160] (signed 32-bit words); output a[256] (signed 32-bit words) */
extern void mldsa_intt_arm(int32_t a[256], const int32_t z_78[384], const int32_t z_123456[160]);

/* Inverse number-theoretic transform for ML-DSA */
/* Input a[256], zetas[624] (signed 32-bit words); output a[256] (signed 32-bit words) */
extern void mldsa_intt(int32_t a[256], const int32_t zetas[624]);

/* Forward number-theoretic transform for ML-DSA */
/* Input a[256], zetas[624] (signed 32-bit words); output a[256] (signed 32-bit words) */
extern void mldsa_ntt(int32_t a[256], const int32_t zetas[624]);

/* NTT unpack for ML-DSA (rearrange coefficients from bitreversed to standard order) */
/* Input a[256] (signed 32-bit words); output a[256] (signed 32-bit words) */
extern void mldsa_nttunpack(int32_t a[256]);

/* Uniform rejection sampling for ML-DSA: extract 23-bit coefficients from */
/* 3-byte-packed input, keeping only those strictly less than q = 8380417. */
/* Returns the number of accepted coefficients (at most 256). */
/* Inputs buf[840] (uint8_t), table[256] (uint64_t lookup table); */
/* output r[256] (int32_t). */
extern uint32_t mldsa_rej_uniform_VARIABLE_TIME_x86(int32_t r[256], const uint8_t buf[840], const uint64_t table[256]);

/* Pointwise multiplication of polynomials in NTT domain (Montgomery form) for ML-DSA */
/* Inputs a[256], b[256] (signed 32-bit words); output r[256] (signed 32-bit words) */
extern void mldsa_pointwise(int32_t r[256], const int32_t a[256], const int32_t b[256]);

/* Pointwise multiplication of polynomials in NTT domain (Montgomery form) for ML-DSA, x86 version */
/* Inputs a[256], b[256], qdata[16] (signed 32-bit words); output c[256] (signed 32-bit words) */
extern void mldsa_pointwise_x86(int32_t c[256], const int32_t a[256], const int32_t b[256], const int32_t qdata[16]);

/* Pointwise multiplication with accumulation for ML-DSA L4 */
/* Inputs a[1024], b[1024] (signed 32-bit words); output r[256] (signed 32-bit words) */
extern void mldsa_pointwise_acc_l4(int32_t r[256], const int32_t a[1024], const int32_t b[1024]);

/* Pointwise multiplication with accumulation for ML-DSA L4, x86 version */
/* Inputs a[1024], b[1024], qdata[16] (signed 32-bit words); output c[256] (signed 32-bit words) */
extern void mldsa_pointwise_acc_l4_x86(int32_t c[256], const int32_t a[1024], const int32_t b[1024], const int32_t qdata[16]);

/* Pointwise multiplication with accumulation for ML-DSA L5 */
/* Inputs a[1280], b[1280] (signed 32-bit words); output r[256] (signed 32-bit words) */
extern void mldsa_pointwise_acc_l5(int32_t r[256], const int32_t a[1280], const int32_t b[1280]);

/* Pointwise multiplication with accumulation for ML-DSA L5, x86 version */
/* Inputs a[1280], b[1280], qdata[16] (signed 32-bit words); output c[256] (signed 32-bit words) */
extern void mldsa_pointwise_acc_l5_x86(int32_t c[256], const int32_t a[1280], const int32_t b[1280], const int32_t qdata[16]);

/* Pointwise multiplication with accumulation for ML-DSA L7 */
/* Inputs a[1792], b[1792] (signed 32-bit words); output r[256] (signed 32-bit words) */
extern void mldsa_pointwise_acc_l7(int32_t r[256], const int32_t a[1792], const int32_t b[1792]);

/* Pointwise multiplication with accumulation for ML-DSA L7, x86 version */
/* Inputs a[1792], b[1792], qdata[16] (signed 32-bit words); output c[256] (signed 32-bit words) */
extern void mldsa_pointwise_acc_l7_x86(int32_t c[256], const int32_t a[1792], const int32_t b[1792], const int32_t qdata[16]);
/* Conditional addition of Q to polynomial coefficients for ML-DSA */
/* Input a[256] (signed 32-bit words); output a[256] (signed 32-bit words) */
extern void mldsa_caddq(int32_t a[256]);

/* Infinity-norm check of polynomial coefficients for ML-DSA */
/* Returns 1 if any coefficient has absolute value >= bound, 0 otherwise */
/* Input a[256] (signed 32-bit words), bound (unsigned 32-bit) */
extern uint64_t mldsa_chknorm(const int32_t a[256], uint64_t bound);

/* Coefficient decomposition for ML-DSA (GAMMA2 = (Q-1)/32, parameter sets 65/87) */
/* Input a0[256] (signed 32-bit words); outputs a1[256] (high parts) and a0[256] (low parts) */
extern void mldsa_decompose_32(int32_t a1[256], int32_t a0[256]);

/* Coefficient decomposition for ML-DSA (GAMMA2 = (Q-1)/88, parameter set 44) */
/* Input a0[256] (signed 32-bit words); outputs a1[256] (high parts) and a0[256] (low parts) */
extern void mldsa_decompose_88(int32_t a1[256], int32_t a0[256]);

/* Unpack packed z polynomial for ML-DSA (GAMMA1 = 2^17, parameter set 44) */
/* Inputs b[576], t[64] (bytes); output r[256] (signed 32-bit words) */
extern void mldsa_polyz_unpack_17_arm(int32_t r[256], const uint8_t b[576], const uint8_t t[64]);
/* x86 variant unpacks directly from the byte buffer (no shuffle table argument) */
/* Input b[576] (bytes); output r[256] (signed 32-bit words) */
extern void mldsa_polyz_unpack_17_x86(int32_t r[256], const uint8_t b[576]);

/* Unpack packed z polynomial for ML-DSA (GAMMA1 = 2^19, parameter sets 65/87) */
/* Inputs b[640], t[64] (bytes); output r[256] (signed 32-bit words) */
extern void mldsa_polyz_unpack_19_arm(int32_t r[256], const uint8_t b[640], const uint8_t t[64]);
/* x86 variant unpacks directly from the byte buffer (no shuffle table argument) */
/* Input b[640] (bytes); output r[256] (signed 32-bit words) */
extern void mldsa_polyz_unpack_19_x86(int32_t r[256], const uint8_t b[640]);

/* Canonical reduction of polynomial coefficients for ML-DSA */
/* Result is centered, -6283009 <= r <= 6283008, and congruent mod 8380417 */
/* Assumes each coefficient is <= 0x7fbfffff (else the reduction overflows) */
/* Input a[256] (signed 32-bit words); output a[256] (signed 32-bit words) */
extern void mldsa_reduce(int32_t a[256]);

/* Use hint to correct high bits of decomposition for ML-DSA (parameter sets 65/87) */
/* The x86 variant operates in place on a[256]; the ARM variant writes b[256] */
#ifdef __x86_64__
/* Inputs a[256], h[256] (signed 32-bit words); output a[256] (signed 32-bit words, in place) */
extern void mldsa_poly_use_hint_32(int32_t a[256], const int32_t h[256]);
#else
/* Inputs a[256], h[256] (signed 32-bit words); output b[256] (signed 32-bit words) */
extern void mldsa_poly_use_hint_32(int32_t b[256], const int32_t a[256], const int32_t h[256]);
#endif

/* Use hint to correct high bits of decomposition for ML-DSA (parameter set 44) */
/* The x86 variant operates in place on a[256]; the ARM variant writes b[256] */
#ifdef __x86_64__
/* Inputs a[256], h[256] (signed 32-bit words); output a[256] (signed 32-bit words, in place) */
extern void mldsa_poly_use_hint_88(int32_t a[256], const int32_t h[256]);
#else
/* Inputs a[256], h[256] (signed 32-bit words); output b[256] (signed 32-bit words) */
extern void mldsa_poly_use_hint_88(int32_t b[256], const int32_t a[256], const int32_t h[256]);
#endif

/* Rejection sampling for ML-DSA secret key (eta = 2; parameter sets 44/87) */
/* The x86 (AVX2) backend has a divergent interface from the Arm (NEON) one: */
/* x86 reads a fixed-size input buffer and a 2048-byte table, so it uses an */
/* _x86-suffixed symbol; Arm takes a variable buflen and a 4096-byte table. */
/* Inputs buf[buflen], buflen, table[4096] (uint8_t); output r[256] (signed 32-bit words) */
extern uint64_t mldsa_rej_uniform_eta2_VARIABLE_TIME(int32_t r[256], const uint8_t *buf, unsigned buflen, const uint8_t table[4096]);
/* Inputs buf[136], table[2048] (uint8_t); output r[256] (signed 32-bit words) */
extern uint64_t mldsa_rej_uniform_eta2_VARIABLE_TIME_x86(int32_t r[256], const uint8_t buf[136], const uint8_t table[2048]);

/* Rejection sampling for ML-DSA secret key (eta = 4; parameter set 65) */
/* The x86 (AVX2) backend has a divergent interface from the Arm (NEON) one: */
/* x86 reads a fixed-size input buffer and a 2048-byte table, so it uses an */
/* _x86-suffixed symbol; Arm takes a variable buflen and a 4096-byte table. */
/* Inputs buf[buflen], buflen, table[4096] (uint8_t); output r[256] (signed 32-bit words) */
extern uint64_t mldsa_rej_uniform_eta4_VARIABLE_TIME(int32_t r[256], const uint8_t *buf, unsigned buflen, const uint8_t table[4096]);
/* Inputs buf[272], table[2048] (uint8_t); output r[256] (signed 32-bit words) */
extern uint64_t mldsa_rej_uniform_eta4_VARIABLE_TIME_x86(int32_t r[256], const uint8_t buf[272], const uint8_t table[2048]);

/* Uniform rejection sampling for ML-DSA */
/* Inputs *buf (unsigned bytes), buflen, table (unsigned bytes); output r[256] (signed 32-bit words), return */
extern uint64_t mldsa_rej_uniform_VARIABLE_TIME(int32_t r[256],const uint8_t *buf,uint64_t buflen,const uint8_t *table);

/* Scalar product of 2-element polynomial vectors in NTT domain, with mulcache */
/* Inputs a[512], b[512], bt[256] (signed 16-bit words); output r[256] (signed 16-bit words) */
extern void mlkem_basemul_k2(int16_t r[256],const int16_t a[512],const int16_t b[512],const int16_t bt[256]);

/* Scalar product of 3-element polynomial vectors in NTT domain, with mulcache */
/* Inputs a[768], b[768], bt[384] (signed 16-bit words); output r[256] (signed 16-bit words) */
extern void mlkem_basemul_k3(int16_t r[256],const int16_t a[768],const int16_t b[768],const int16_t bt[384]);

/* Scalar product of 4-element polynomial vectors in NTT domain, with mulcache */
/* Inputs a[1024], b[1024], bt[512] (signed 16-bit words); output r[256] (signed 16-bit words) */
extern void mlkem_basemul_k4(int16_t r[256],const int16_t a[1024],const int16_t b[1024],const int16_t bt[512]);

/* Inverse number-theoretic transform from ML-KEM */
/* Input a[256] (signed 16-bit words), z_01234[80] (signed 16-bit words), z_56[384] (signed 16-bit words); output a[256] (signed 16-bit words) */
extern void mlkem_intt(int16_t a[256],const int16_t z_01234[80],const int16_t z_56[384]);

/* Inverse number-theoretic transform from ML-KEM */
/* Input a[256] (signed 16-bit words), qdata[624]; output a[256] (signed 16-bit words) */
extern void mlkem_intt_x86(int16_t a[256],const int16_t qdata[624]);

/* Precompute the mulcache data for a polynomial in the NTT domain */
/* Inputs a[256], z[128] and t[128] (signed 16-bit words); output x[128] (signed 16-bit words) */
extern void mlkem_mulcache_compute(int16_t x[128],const int16_t a[256],const int16_t z[128],const int16_t t[128]);

/* Precompute the mulcache data for a polynomial in the NTT domain */
/* Inputs a[256], qdata[624] (signed 16-bit words); output x[128] (signed 16-bit words) */
extern void mlkem_mulcache_compute_x86(int16_t x[128],const int16_t a[256],const int16_t qdata[624]);

/* Forward number-theoretic transform from ML-KEM x86 implementation */
/* Input a[256] (signed 16-bit words), qdata[624]; output a[256] (signed 16-bit words) */
extern void mlkem_ntt_x86(int16_t a[256],const int16_t qdata[624]);

/* Forward number-theoretic transform from ML-KEM */
/* Input a[256] (signed 16-bit words), z_01234[80] (signed 16-bit words), z_56[384] (signed 16-bit words); output a[256] (signed 16-bit words) */
extern void mlkem_ntt(int16_t a[256],const int16_t z_01234[80],const int16_t z_56[384]);

/* Canonical modular reduction of polynomial coefficients for ML-KEM */
/* Input a[256] (signed 16-bit words); output a[256] (signed 16-bit words) */
extern void mlkem_reduce(int16_t a[256]);

/* Unpack ML-KEM polynomial coefficients */
/* Input a[384] (bytes); output r[256] (signed 16-bit words) */
extern void mlkem_frombytes(int16_t r[256],const uint8_t a[384]);

/* Pack ML-KEM polynomial coefficients as 12-bit numbers */
/* Input a[256] (signed 16-bit words); output r[384] (bytes) */
extern void mlkem_tobytes(uint8_t r[384],const int16_t a[256]);

/* Conversion of ML-KEM polynomial coefficients to Montgomery form */
/* Input a[256] (signed 16-bit words); output a[256] (signed 16-bit words) */
extern void mlkem_tomont(int16_t a[256]);

/* Uniform rejection sampling for ML-KEM */
/* Inputs *buf (unsigned bytes), buflen, table (unsigned bytes); output r[256] (signed 16-bit words), return */
extern uint64_t mlkem_rej_uniform_VARIABLE_TIME(int16_t r[256],const uint8_t *buf,uint64_t buflen,const uint8_t *table);

/* Reorders ML-KEM polynomial coefficients for x86 implementation */
/* Input a[256] (signed 16-bit words); output a[256] (signed 16-bit words) */
extern void mlkem_unpack(int16_t a[256]);

/* Point addition on NIST curve P-256 in Montgomery-Jacobian coordinates */
/* Inputs p1[12], p2[12]; output p3[12] */
extern void p256_montjadd(uint64_t p3[12],const uint64_t p1[12],const uint64_t p2[12]);
extern void p256_montjadd_alt(uint64_t p3[12],const uint64_t p1[12],const uint64_t p2[12]);

/* Point doubling on NIST curve P-256 in Montgomery-Jacobian coordinates */
/* Inputs p1[12]; output p3[12] */
extern void p256_montjdouble(uint64_t p3[12],const uint64_t p1[12]);
extern void p256_montjdouble_alt(uint64_t p3[12],const uint64_t p1[12]);

/* Point mixed addition on NIST curve P-256 in Montgomery-Jacobian coordinates */
/* Inputs p1[12], p2[8]; output p3[12] */
extern void p256_montjmixadd(uint64_t p3[12],const uint64_t p1[12],const uint64_t p2[8]);
extern void p256_montjmixadd_alt(uint64_t p3[12],const uint64_t p1[12],const uint64_t p2[8]);

/* Montgomery-Jacobian form scalar multiplication for P-256 */
/* Input scalar[4], point[12]; output res[12] */
extern void p256_montjscalarmul(uint64_t res[12],const uint64_t scalar[4],const uint64_t point[12]);
extern void p256_montjscalarmul_alt(uint64_t res[12],const uint64_t scalar[4],const uint64_t point[12]);

/* Scalar multiplication for NIST curve P-256 */
/* Input scalar[4], point[8]; output res[8] */
extern void p256_scalarmul(uint64_t res[8],const uint64_t scalar[4],const uint64_t point[8]);
extern void p256_scalarmul_alt(uint64_t res[8],const uint64_t scalar[4],const uint64_t point[8]);

/* Scalar multiplication for precomputed point on NIST curve P-256 */
/* Input scalar[4], blocksize, table[]; output res[8] */
extern void p256_scalarmulbase(uint64_t res[8],const uint64_t scalar[4],uint64_t blocksize,const uint64_t *table);
extern void p256_scalarmulbase_alt(uint64_t res[8],const uint64_t scalar[4],uint64_t blocksize,const uint64_t *table);

/* Point addition on NIST curve P-384 in Montgomery-Jacobian coordinates */
/* Inputs p1[18], p2[18]; output p3[18] */
extern void p384_montjadd(uint64_t p3[18],const uint64_t p1[18],const uint64_t p2[18]);
extern void p384_montjadd_alt(uint64_t p3[18],const uint64_t p1[18],const uint64_t p2[18]);

/* Point doubling on NIST curve P-384 in Montgomery-Jacobian coordinates */
/* Inputs p1[18]; output p3[18] */
extern void p384_montjdouble(uint64_t p3[18],const uint64_t p1[18]);
extern void p384_montjdouble_alt(uint64_t p3[18],const uint64_t p1[18]);

/* Point mixed addition on NIST curve P-384 in Montgomery-Jacobian coordinates */
/* Inputs p1[18], p2[12]; output p3[18] */
extern void p384_montjmixadd(uint64_t p3[18],const uint64_t p1[18],const uint64_t p2[12]);
extern void p384_montjmixadd_alt(uint64_t p3[18],const uint64_t p1[18],const uint64_t p2[12]);

/* Montgomery-Jacobian form scalar multiplication for P-384 */
/* Input scalar[6], point[18]; output res[18] */
extern void p384_montjscalarmul(uint64_t res[18],const uint64_t scalar[6],const uint64_t point[18]);
extern void p384_montjscalarmul_alt(uint64_t res[18],const uint64_t scalar[6],const uint64_t point[18]);

/* Point addition on NIST curve P-521 in Jacobian coordinates */
/* Inputs p1[27], p2[27]; output p3[27] */
extern void p521_jadd(uint64_t p3[27],const uint64_t p1[27],const uint64_t p2[27]);
extern void p521_jadd_alt(uint64_t p3[27],const uint64_t p1[27],const uint64_t p2[27]);

/* Point doubling on NIST curve P-521 in Jacobian coordinates */
/* Input p1[27]; output p3[27] */
extern void p521_jdouble(uint64_t p3[27],const uint64_t p1[27]);
extern void p521_jdouble_alt(uint64_t p3[27],const uint64_t p1[27]);

/* Point mixed addition on NIST curve P-521 in Jacobian coordinates */
/* Inputs p1[27], p2[18]; output p3[27] */
extern void p521_jmixadd(uint64_t p3[27],const uint64_t p1[27],const uint64_t p2[18]);
extern void p521_jmixadd_alt(uint64_t p3[27],const uint64_t p1[27],const uint64_t p2[18]);

/* Jacobian form scalar multiplication for P-521 */
/* Input scalar[9], point[27]; output res[27] */
extern void p521_jscalarmul(uint64_t res[27],const uint64_t scalar[9],const uint64_t point[27]);
extern void p521_jscalarmul_alt(uint64_t res[27],const uint64_t scalar[9],const uint64_t point[27]);

/* Point addition on SECG curve secp256k1 in Jacobian coordinates */
/* Inputs p1[12], p2[12]; output p3[12] */
extern void secp256k1_jadd(uint64_t p3[12],const uint64_t p1[12],const uint64_t p2[12]);
extern void secp256k1_jadd_alt(uint64_t p3[12],const uint64_t p1[12],const uint64_t p2[12]);

/* Point doubling on SECG curve secp256k1 in Jacobian coordinates */
/* Input p1[12]; output p3[12] */
extern void secp256k1_jdouble(uint64_t p3[12],const uint64_t p1[12]);
extern void secp256k1_jdouble_alt(uint64_t p3[12],const uint64_t p1[12]);

/* Point mixed addition on SECG curve secp256k1 in Jacobian coordinates */
/* Inputs p1[12], p2[8]; output p3[12] */
extern void secp256k1_jmixadd(uint64_t p3[12],const uint64_t p1[12],const uint64_t p2[8]);
extern void secp256k1_jmixadd_alt(uint64_t p3[12],const uint64_t p1[12],const uint64_t p2[8]);

/* Keccak-f1600 permutation for SHA3 */
/* Inputs a[25], rc[24]; output a[25] */
extern void sha3_keccak_f1600(uint64_t a[25],const uint64_t rc[24]);
extern void sha3_keccak_f1600_alt(uint64_t a[25],const uint64_t rc[24]);
extern void sha3_keccak_f1600_alt2(uint64_t a[25],const uint64_t rc[24]);

/* Batched 2-way Keccak-f1600 permutation for SHA3 */
/* Inputs a[50], rc[24]; output a[50] */
extern void sha3_keccak2_f1600(uint64_t a[50],const uint64_t rc[24]);
extern void sha3_keccak2_f1600_alt(uint64_t a[50],const uint64_t rc[24]);

/* Batched 4-way Keccak-f1600 permutation for SHA3 */
/* Inputs a[100], rc[24]; output a[100] */
extern void sha3_keccak4_f1600(uint64_t a[100],const uint64_t rc[24]);
#ifdef __x86_64__
extern void sha3_keccak4_f1600_alt(uint64_t a[100],const uint64_t rc[24],const uint64_t rho8[4],const uint64_t rho56[4]);
#else
extern void sha3_keccak4_f1600_alt(uint64_t a[100],const uint64_t rc[24]);
#endif
extern void sha3_keccak4_f1600_alt2(uint64_t a[100],const uint64_t rc[24]);

/* Point addition on CC curve SM2 in Montgomery-Jacobian coordinates */
/* Inputs p1[12], p2[12]; output p3[12] */
extern void sm2_montjadd(uint64_t p3[12],const uint64_t p1[12],const uint64_t p2[12]);
extern void sm2_montjadd_alt(uint64_t p3[12],const uint64_t p1[12],const uint64_t p2[12]);

/* Point doubling on CC curve SM2 in Montgomery-Jacobian coordinates */
/* Inputs p1[12]; output p3[12] */
extern void sm2_montjdouble(uint64_t p3[12],const uint64_t p1[12]);
extern void sm2_montjdouble_alt(uint64_t p3[12],const uint64_t p1[12]);

/* Point mixed addition on CC curve SM2 in Montgomery-Jacobian coordinates */
/* Inputs p1[12], p2[8]; output p3[12] */
extern void sm2_montjmixadd(uint64_t p3[12],const uint64_t p1[12],const uint64_t p2[8]);
extern void sm2_montjmixadd_alt(uint64_t p3[12],const uint64_t p1[12],const uint64_t p2[8]);

/* Montgomery-Jacobian form scalar multiplication for CC curve SM2 */
/* Input scalar[4], point[12]; output res[12] */
extern void sm2_montjscalarmul(uint64_t res[12],const uint64_t scalar[4],const uint64_t point[12]);
extern void sm2_montjscalarmul_alt(uint64_t res[12],const uint64_t scalar[4],const uint64_t point[12]);

/* Reverse the bytes in a single word */
/* Input a; output function return */
extern uint64_t word_bytereverse (uint64_t a);

/* Count leading zero bits in a single word */
/* Input a; output function return */
extern uint64_t word_clz (uint64_t a);

/* Count trailing zero bits in a single word */
/* Input a; output function return */
extern uint64_t word_ctz (uint64_t a);

/* Perform 59 "divstep" iterations and return signed matrix of updates */
/* Inputs d, f, g; output m[2][2] and function return */
extern int64_t word_divstep59(int64_t m[2][2],int64_t d,uint64_t f,uint64_t g);

/* Return maximum of two unsigned 64-bit words */
/* Inputs a, b; output function return */
extern uint64_t word_max (uint64_t a, uint64_t b);

/* Return minimum of two unsigned 64-bit words */
/* Inputs a, b; output function return */
extern uint64_t word_min (uint64_t a, uint64_t b);

/* Single-word negated modular inverse (-1/a) mod 2^64 */
/* Input a; output function return */
extern uint64_t word_negmodinv (uint64_t a);

/* Count number of set bits in a single 64-bit word (population count) */
/* Input a; output function return */
extern uint64_t word_popcount (uint64_t a);

/* Single-word reciprocal, 2^64 + ret = ceil(2^128/a) - 1 if MSB of "a" is set */
/* Input a; output function return */
extern uint64_t word_recip (uint64_t a);
