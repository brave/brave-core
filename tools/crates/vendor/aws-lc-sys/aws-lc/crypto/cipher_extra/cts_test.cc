// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

#include <openssl/aes.h>
#include <openssl/modes.h>
#include <openssl/rand.h>

#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "../test/test_util.h"


// Each case fixes |key|, the all-zero IV, and the |plaintext| / |ciphertext|
// pair produced by encrypting under CBC-CS3 (the OpenSSL legacy convention).
// All inputs are >= 17 bytes (CTS requires more than one block).
struct CTSTestCase {
  const char *name;
  const char *key_hex;
  const char *plaintext_hex;
  const char *ciphertext_hex;
};

// Vectors derived from RFC 8009 Appendix A. Each "AES Output" entry there is
// CBC-CTS(Ke, IV=0, confounder || plaintext); we use that as the ciphertext
// expectation, with input = confounder || plaintext.
//
//   https://datatracker.ietf.org/doc/html/rfc8009#appendix-A
//
// RFC 8009 specifies CBC-CS3 (always-swap), which is exactly the convention
// the OpenSSL legacy |CRYPTO_cts128_encrypt| API implements: |residue == 0|
// is rewritten to |residue == 16| so the swap always happens. krb5 1.x calls
// into AWS-LC through this same legacy path.
static const CTSTestCase kRFC8009Cases[] = {
    // AES-128: 6-byte plaintext residue (input = 16-byte confounder + 6 bytes,
    // total 22 bytes; 6-byte trailing residue after the first full block).
    {"AES-128, 22-byte input (residue 6)",
     "9B197DD1E8C5609D6E67C3E37C62C72E",
     "7BCA285E2FD4130FB55B1A5C83BC5B24"
     "000102030405",
     "84D7F30754ED987BAB0BF3506BEB09CF"
     "B55402CEF7E6"},
    // AES-128: exact 32-byte input (residue is 0 mod 16; CS3 still swaps).
    {"AES-128, 32-byte input (exact 2 blocks)",
     "9B197DD1E8C5609D6E67C3E37C62C72E",
     "56AB21713FF62C0A1457200F6FA9948F"
     "000102030405060708090A0B0C0D0E0F",
     "3517D640F50DDC8AD3628722B3569D2A"
     "E07493FA8263254080EA65C1008E8FC2"},
    // AES-128: 37-byte input (residue 5 after two full blocks).
    {"AES-128, 37-byte input (residue 5)",
     "9B197DD1E8C5609D6E67C3E37C62C72E",
     "A7A4E29A4728CE10664FB64E49AD3FAC"
     "000102030405060708090A0B0C0D0E0F"
     "1011121314",
     "720F73B18D9859CD6CCB4346115CD336"
     "C70F58EDC0C4437C5573544C31C813BC"
     "E1E6D072C1"},
    // AES-256: 22-byte input (residue 6).
    {"AES-256, 22-byte input (residue 6)",
     "56AB22BEE63D82D7BC5227F6773F8EA7"
     "A5EB1C825160C38312980C442E5C7E49",
     "B80D3251C1F6471494256FFE712D0B9A"
     "000102030405",
     "4ED7B37C2BCAC8F74F23C1CF07E62BC7"
     "B75FB3F637B9"},
    // AES-256: 32-byte input (exact 2 blocks; CS3 always swaps).
    {"AES-256, 32-byte input (exact 2 blocks)",
     "56AB22BEE63D82D7BC5227F6773F8EA7"
     "A5EB1C825160C38312980C442E5C7E49",
     "53BF8A0D105265D4E276428624CE5E63"
     "000102030405060708090A0B0C0D0E0F",
     "BC47FFEC7998EB91E8115CF8D19DAC4B"
     "BBE2E163E87DD37F49BECA92027764F6"},
    // AES-256: 37-byte input (residue 5).
    {"AES-256, 37-byte input (residue 5)",
     "56AB22BEE63D82D7BC5227F6773F8EA7"
     "A5EB1C825160C38312980C442E5C7E49",
     "763E65367E864F02F55153C7E3B58AF1"
     "000102030405060708090A0B0C0D0E0F"
     "1011121314",
     "40013E2DF58E8751957D2878BCD2D6FE"
     "101CCFD556CB1EAE79DB3C3EE86429F2"
     "B2A602AC86"},
};

// RFC 3962 Appendix B test vectors. Same AES-128 key ("chicken teriyaki"),
// IV = all zeros, progressively longer plaintexts ("I would like the...").
// These are the canonical Kerberos AES-CTS test vectors.
static const CTSTestCase kRFC3962Cases[] = {
    {"RFC3962: 17 bytes",
     "636869636B656E207465726979616B69",
     "4920776F756C64206C696B652074686520",
     "C6353568F2BF8CB4D8A580362DA7FF7F97"},
    {"RFC3962: 31 bytes",
     "636869636B656E207465726979616B69",
     "4920776F756C64206C696B652074686520"
     "47656E6572616C20476175277320",
     "FC00783E0EFDB2C1D445D4C8EFF7ED22"
     "97687268D6ECCCC0C07B25E25ECFE5"},
    {"RFC3962: 32 bytes",
     "636869636B656E207465726979616B69",
     "4920776F756C64206C696B652074686520"
     "47656E6572616C2047617527732043",
     "39312523A78662D5BE7FCBCC98EBF5A8"
     "97687268D6ECCCC0C07B25E25ECFE584"},
    {"RFC3962: 47 bytes",
     "636869636B656E207465726979616B69",
     "4920776F756C64206C696B652074686520"
     "47656E6572616C20476175277320436869"
     "636B656E2C20706C656173652C",
     "97687268D6ECCCC0C07B25E25ECFE584"
     "B3FFFD940C16A18C1B5549D2F838029E"
     "39312523A78662D5BE7FCBCC98EBF5"},
    {"RFC3962: 48 bytes",
     "636869636B656E207465726979616B69",
     "4920776F756C64206C696B652074686520"
     "47656E6572616C20476175277320436869"
     "636B656E2C20706C656173652C20",
     "97687268D6ECCCC0C07B25E25ECFE584"
     "9DAD8BBB96C4CDC03BC103E1A194BBD8"
     "39312523A78662D5BE7FCBCC98EBF5A8"},
    {"RFC3962: 64 bytes",
     "636869636B656E207465726979616B69",
     "4920776F756C64206C696B652074686520"
     "47656E6572616C20476175277320436869"
     "636B656E2C20706C656173652C20616E64"
     "20776F6E746F6E20736F75702E",
     "97687268D6ECCCC0C07B25E25ECFE584"
     "39312523A78662D5BE7FCBCC98EBF5A8"
     "4807EFE836EE89A526730DBC2F7BC840"
     "9DAD8BBB96C4CDC03BC103E1A194BBD8"},
};

// Decode like HexToBytes, but tolerate spaces (used to keep the literals above
// readable line-by-line).
static std::vector<uint8_t> DecodeHexSp(const std::string &in) {
  std::string filtered;
  filtered.reserve(in.size());
  for (char c : in) {
    if (c != ' ') {
      filtered.push_back(c);
    }
  }
  std::vector<uint8_t> out;
  if (!DecodeHex(&out, filtered)) {
    ADD_FAILURE() << "Bad hex literal: '" << in << "'";
  }
  return out;
}

TEST(CTS128Test, RFC3962Vectors) {
  for (const auto &t : kRFC3962Cases) {
    SCOPED_TRACE(t.name);
    const auto key_bytes = DecodeHexSp(t.key_hex);
    const auto pt = DecodeHexSp(t.plaintext_hex);
    const auto ct = DecodeHexSp(t.ciphertext_hex);
    ASSERT_GT(pt.size(), size_t{16}) << "CTS requires >16-byte input";
    ASSERT_EQ(pt.size(), ct.size());

    AES_KEY enc_key, dec_key;
    ASSERT_EQ(0, AES_set_encrypt_key(key_bytes.data(),
                                     static_cast<unsigned>(key_bytes.size() * 8),
                                     &enc_key));
    ASSERT_EQ(0, AES_set_decrypt_key(key_bytes.data(),
                                     static_cast<unsigned>(key_bytes.size() * 8),
                                     &dec_key));

    std::vector<uint8_t> got_ct(pt.size());
    uint8_t iv[16] = {0};
    size_t n = CRYPTO_cts128_encrypt(pt.data(), got_ct.data(), pt.size(),
                                     &enc_key, iv, AES_cbc_encrypt);
    EXPECT_EQ(pt.size(), n);
    EXPECT_EQ(Bytes(ct.data(), ct.size()), Bytes(got_ct.data(), got_ct.size()));

    std::vector<uint8_t> got_pt(ct.size());
    uint8_t iv2[16] = {0};
    n = CRYPTO_cts128_decrypt(ct.data(), got_pt.data(), ct.size(), &dec_key,
                              iv2, AES_cbc_encrypt);
    EXPECT_EQ(ct.size(), n);
    EXPECT_EQ(Bytes(pt.data(), pt.size()), Bytes(got_pt.data(), got_pt.size()));
  }
}

TEST(CTS128Test, RFC8009RoundTrip) {
  for (const auto &t : kRFC8009Cases) {
    SCOPED_TRACE(t.name);
    const auto key_bytes = DecodeHexSp(t.key_hex);
    const auto pt = DecodeHexSp(t.plaintext_hex);
    const auto ct = DecodeHexSp(t.ciphertext_hex);
    ASSERT_GT(pt.size(), size_t{16}) << "CTS requires >16-byte input";
    ASSERT_EQ(pt.size(), ct.size());

    AES_KEY enc_key, dec_key;
    ASSERT_EQ(0, AES_set_encrypt_key(key_bytes.data(),
                                     static_cast<unsigned>(key_bytes.size() * 8),
                                     &enc_key));
    ASSERT_EQ(0, AES_set_decrypt_key(key_bytes.data(),
                                     static_cast<unsigned>(key_bytes.size() * 8),
                                     &dec_key));

    // Encrypt against the published ciphertext.
    std::vector<uint8_t> got_ct(pt.size());
    uint8_t iv[16] = {0};
    size_t n = CRYPTO_cts128_encrypt(pt.data(), got_ct.data(), pt.size(),
                                     &enc_key, iv, AES_cbc_encrypt);
    EXPECT_EQ(pt.size(), n);
    EXPECT_EQ(Bytes(ct.data(), ct.size()), Bytes(got_ct.data(), got_ct.size()));

    // Round-trip decrypt back to plaintext.
    std::vector<uint8_t> got_pt(ct.size());
    uint8_t iv2[16] = {0};
    n = CRYPTO_cts128_decrypt(ct.data(), got_pt.data(), ct.size(), &dec_key,
                              iv2, AES_cbc_encrypt);
    EXPECT_EQ(ct.size(), n);
    EXPECT_EQ(Bytes(pt.data(), pt.size()), Bytes(got_pt.data(), got_pt.size()));
  }
}

// Exhaustively walk every input length we can practically hit: from 17 bytes
// (the smallest legal CTS input) through 5 full blocks plus all residues, for
// AES-128, AES-192, and AES-256. Verifies the encrypt-then-decrypt round-trip
// is the identity at every length boundary.
TEST(CTS128Test, RoundTripAllLengthsAllKeySizes) {
  static const unsigned kKeyBits[] = {128, 192, 256};
  static const size_t kMinLen = 17;
  static const size_t kMaxLen = 16 * 5 + 15;

  for (unsigned bits : kKeyBits) {
    SCOPED_TRACE(testing::Message() << "AES-" << bits);
    std::vector<uint8_t> key_bytes(bits / 8);
    ASSERT_TRUE(RAND_bytes(key_bytes.data(), key_bytes.size()));

    AES_KEY enc_key, dec_key;
    ASSERT_EQ(0, AES_set_encrypt_key(key_bytes.data(), bits, &enc_key));
    ASSERT_EQ(0, AES_set_decrypt_key(key_bytes.data(), bits, &dec_key));

    for (size_t len = kMinLen; len <= kMaxLen; len++) {
      SCOPED_TRACE(testing::Message() << "len=" << len);
      std::vector<uint8_t> pt(len);
      std::vector<uint8_t> iv_seed(16);
      ASSERT_TRUE(RAND_bytes(pt.data(), pt.size()));
      ASSERT_TRUE(RAND_bytes(iv_seed.data(), iv_seed.size()));

      // Encrypt.
      std::vector<uint8_t> ct(len);
      uint8_t iv[16];
      memcpy(iv, iv_seed.data(), 16);
      size_t n = CRYPTO_cts128_encrypt(pt.data(), ct.data(), len, &enc_key, iv,
                                       AES_cbc_encrypt);
      ASSERT_EQ(len, n);

      // Decrypt round-trip.
      std::vector<uint8_t> pt2(len);
      memcpy(iv, iv_seed.data(), 16);
      n = CRYPTO_cts128_decrypt(ct.data(), pt2.data(), len, &dec_key, iv,
                                AES_cbc_encrypt);
      ASSERT_EQ(len, n);
      EXPECT_EQ(Bytes(pt.data(), len), Bytes(pt2.data(), len));
    }
  }
}

TEST(CTS128Test, RejectsTooShortInput) {
  // CTS is undefined for |len| <= 16; OpenSSL signals this by returning 0.
  uint8_t key_bytes[16] = {0};
  AES_KEY enc_key, dec_key;
  ASSERT_EQ(0, AES_set_encrypt_key(key_bytes, 128, &enc_key));
  ASSERT_EQ(0, AES_set_decrypt_key(key_bytes, 128, &dec_key));

  uint8_t buf_in[16] = {0};
  uint8_t buf_out[16] = {0};
  uint8_t iv[16] = {0};

  for (size_t len : {size_t{0}, size_t{1}, size_t{15}, size_t{16}}) {
    SCOPED_TRACE(testing::Message() << "len=" << len);
    EXPECT_EQ(size_t{0},
              CRYPTO_cts128_encrypt(buf_in, buf_out, len, &enc_key, iv,
                                    AES_cbc_encrypt));
    EXPECT_EQ(size_t{0},
              CRYPTO_cts128_decrypt(buf_in, buf_out, len, &dec_key, iv,
                                    AES_cbc_encrypt));
  }
}

TEST(CTS128Test, AgreesWithRawCBCOnExactBlockMultiple) {
  // CTS at residue==16 should agree with running plain CBC over the whole
  // input and then swapping the final two blocks (i.e. the CS3 convention).
  uint8_t key_bytes[16];
  ASSERT_TRUE(RAND_bytes(key_bytes, sizeof(key_bytes)));
  AES_KEY enc_key, dec_key;
  ASSERT_EQ(0, AES_set_encrypt_key(key_bytes, 128, &enc_key));
  ASSERT_EQ(0, AES_set_decrypt_key(key_bytes, 128, &dec_key));

  // Exactly 4 blocks.
  constexpr size_t kLen = 64;
  std::vector<uint8_t> pt(kLen), iv_seed(16);
  ASSERT_TRUE(RAND_bytes(pt.data(), kLen));
  ASSERT_TRUE(RAND_bytes(iv_seed.data(), 16));

  std::vector<uint8_t> cts_ct(kLen);
  uint8_t iv[16];
  memcpy(iv, iv_seed.data(), 16);
  ASSERT_EQ(kLen, CRYPTO_cts128_encrypt(pt.data(), cts_ct.data(), kLen,
                                        &enc_key, iv, AES_cbc_encrypt));

  // Compute reference CBC, then swap the last two blocks.
  std::vector<uint8_t> cbc_ct(kLen);
  memcpy(iv, iv_seed.data(), 16);
  AES_cbc_encrypt(pt.data(), cbc_ct.data(), kLen, &enc_key, iv, AES_ENCRYPT);
  std::vector<uint8_t> swapped(kLen);
  memcpy(swapped.data(), cbc_ct.data(), kLen - 32);
  memcpy(swapped.data() + kLen - 32, cbc_ct.data() + kLen - 16, 16);
  memcpy(swapped.data() + kLen - 16, cbc_ct.data() + kLen - 32, 16);

  EXPECT_EQ(Bytes(swapped.data(), kLen), Bytes(cts_ct.data(), kLen));
}

TEST(CTS128Test, IVUpdatedToLastFullCipherBlock) {
  // After encryption the |ivec| should be advanced to the (post-swap) penultimate
  // ciphertext block, matching OpenSSL's behavior -- that's how krb5 chains CTS
  // calls together for streamed input.
  uint8_t key_bytes[16];
  ASSERT_TRUE(RAND_bytes(key_bytes, sizeof(key_bytes)));
  AES_KEY enc_key;
  ASSERT_EQ(0, AES_set_encrypt_key(key_bytes, 128, &enc_key));

  constexpr size_t kLen = 37;  // residue 5 after 2 full blocks
  std::vector<uint8_t> pt(kLen);
  ASSERT_TRUE(RAND_bytes(pt.data(), kLen));

  std::vector<uint8_t> ct(kLen);
  uint8_t iv[16] = {0};
  ASSERT_EQ(kLen, CRYPTO_cts128_encrypt(pt.data(), ct.data(), kLen, &enc_key,
                                        iv, AES_cbc_encrypt));
  // The post-swap penultimate ciphertext block is at ct[kLen - kLen%16 - 16],
  // i.e. the one that holds a full 16 bytes adjacent to the |residue|-byte
  // tail. After CS3 swap, that's the block originally produced last.
  // CS3 puts the just-encrypted block at ct[len-residue-16 .. len-residue].
  size_t residue = kLen % 16;
  EXPECT_EQ(Bytes(ct.data() + kLen - residue - 16, 16), Bytes(iv, 16));
}
