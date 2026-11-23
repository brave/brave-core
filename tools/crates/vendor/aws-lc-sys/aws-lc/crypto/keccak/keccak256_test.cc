// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

#include <stdio.h>
#include <string.h>

#include <vector>

#include <gtest/gtest.h>

#include <openssl/digest.h>
#include <openssl/evp.h>
#include <openssl/span.h>

#include "internal.h"
#include "../fipsmodule/sha/internal.h"
#include "../internal.h"
#include "../test/file_test.h"
#include "../test/test_util.h"

// Keccak-256 (Ethereum-style, 0x01 padding) and FIPS SHA3-256 share rate and
// digest length but use different padding bytes, so their outputs must always
// differ. This guards against a future maintainer "fixing" the padding.
TEST(Keccak256Test, DiffersFromSHA3_256) {
  static_assert(KECCAK256_DIGEST_LENGTH == SHA3_256_DIGEST_LENGTH,
                "size mismatch invalidates compare");
  const char *kInputs[] = {"", "abc", "keccak", "hello world"};
  for (const char *in : kInputs) {
    SCOPED_TRACE(in);
    const size_t len = strlen(in);
    const uint8_t *data = reinterpret_cast<const uint8_t *>(in);
    uint8_t k[KECCAK256_DIGEST_LENGTH];
    uint8_t s[SHA3_256_DIGEST_LENGTH];
    ASSERT_EQ(k, Keccak256(data, len, k));
    ASSERT_EQ(s, SHA3_256(data, len, s));
    EXPECT_NE(0, OPENSSL_memcmp(k, s, KECCAK256_DIGEST_LENGTH))
        << "Keccak-256 and SHA3-256 must produce different digests";
  }
}

// Misuse of the streaming API must fail cleanly rather than hang or corrupt
// memory. A zeroed context arises whenever |Keccak256_Init| is skipped, and also
// on a second |Keccak256_Final| through EVP, because |EVP_DigestFinal_ex|
// cleanses |md_data|. Such a context must not reach the KeccakSponge
// primitives, which assume it is initialised: |KeccakSponge_AbsorbFinal| would
// index |ctx->buf[block_size - 1]| out of bounds and |Keccak1600_Absorb| would
// loop forever on |r == 0|.
//
// Only |Keccak256| is |OPENSSL_EXPORT|ed, so the streaming primitives are
// unreachable when this test links against the shared library. The EVP-level
// regression test below covers the same guards in every configuration.
#if !defined(BORINGSSL_SHARED_LIBRARY)
TEST(Keccak256Test, MisuseFailsCleanly) {
  uint8_t out[KECCAK256_DIGEST_LENGTH];

  // A second |Keccak256_Final| on a finalised context fails: |ctx->state| is
  // |KECCAK1600_STATE_FINAL|, which |KeccakSponge_AbsorbFinal| rejects.
  {
    KECCAK1600_CTX ctx;
    ASSERT_TRUE(Keccak256_Init(&ctx));
    ASSERT_TRUE(Keccak256_Update(&ctx, "abc", 3));
    ASSERT_TRUE(Keccak256_Final(out, &ctx));
    EXPECT_FALSE(Keccak256_Final(out, &ctx));
    // Absorbing more input after finalising fails for the same reason.
    EXPECT_FALSE(Keccak256_Update(&ctx, "abc", 3));
  }

  // A zeroed context (|Keccak256_Init| skipped) must not reach the KeccakSponge
  // layer. |Keccak256_Final| reports success without writing, matching
  // |SHA3_Final|'s |md_size == 0| guard; |Keccak256_Update| reports failure.
  {
    KECCAK1600_CTX ctx;
    OPENSSL_memset(&ctx, 0, sizeof(ctx));
    EXPECT_FALSE(Keccak256_Update(&ctx, "abc", 3));
    EXPECT_TRUE(Keccak256_Final(out, &ctx));
  }

  // NULL arguments are rejected.
  {
    KECCAK1600_CTX ctx;
    ASSERT_TRUE(Keccak256_Init(&ctx));
    EXPECT_FALSE(Keccak256_Init(nullptr));
    EXPECT_FALSE(Keccak256_Update(nullptr, "abc", 3));
    EXPECT_FALSE(Keccak256_Update(&ctx, nullptr, 3));
    EXPECT_FALSE(Keccak256_Final(out, nullptr));
    EXPECT_FALSE(Keccak256_Final(nullptr, &ctx));
    // A zero-length update is a no-op, so a NULL buffer is tolerated.
    EXPECT_TRUE(Keccak256_Update(&ctx, nullptr, 0));
  }
}
#endif  // !BORINGSSL_SHARED_LIBRARY

// The same misuse through the EVP interface must not hang either. This is the
// path that regressed: |EVP_DigestFinal_ex| cleanses the context, so a second
// call re-enters |Keccak256_Final| with a zeroed context.
TEST(Keccak256Test, EVPDoubleFinal) {
  uint8_t out[KECCAK256_DIGEST_LENGTH];
  unsigned out_len = 0;
  bssl::ScopedEVP_MD_CTX ctx;
  ASSERT_TRUE(EVP_DigestInit_ex(ctx.get(), EVP_keccak256(), nullptr));
  ASSERT_TRUE(EVP_DigestUpdate(ctx.get(), "abc", 3));
  ASSERT_TRUE(EVP_DigestFinal_ex(ctx.get(), out, &out_len));
  ASSERT_EQ(static_cast<unsigned>(KECCAK256_DIGEST_LENGTH), out_len);
  // Matches SHA3-256 and BLAKE2b-256: succeeds without hanging or aborting.
  EXPECT_TRUE(EVP_DigestFinal_ex(ctx.get(), out, &out_len));
}

// File-driven Keccak-256 KAT vectors. Format mirrors NIST SHA-3 KATs
// (Len in bits, Msg/MD in lowercase hex). See sha3_test.cc for the same
// pattern applied to SHA-3.
TEST(Keccak256Test, KAT) {
  auto run = [](FileTest *t) {
    std::string len_str;
    ASSERT_TRUE(t->GetAttribute(&len_str, "Len"));
    int bit_len = 0;
    ASSERT_EQ(1, sscanf(len_str.c_str(), "%d", &bit_len));
    ASSERT_GE(bit_len, 0);
    // The current KAT covers byte-aligned messages only; bit-level inputs
    // need a separate API.
    ASSERT_EQ(0, bit_len % 8) << "Non-byte-aligned KAT vectors are unsupported";
    const size_t byte_len = static_cast<size_t>(bit_len) / 8;

    std::vector<uint8_t> msg, md;
    ASSERT_TRUE(t->GetBytes(&msg, "Msg"));
    ASSERT_TRUE(t->GetBytes(&md, "MD"));
    ASSERT_LE(byte_len, msg.size());
    ASSERT_EQ(static_cast<size_t>(KECCAK256_DIGEST_LENGTH), md.size());

    uint8_t out[KECCAK256_DIGEST_LENGTH];
    ASSERT_EQ(out, Keccak256(msg.data(), byte_len, out));
    EXPECT_EQ(EncodeHex(bssl::MakeConstSpan(md)),
              EncodeHex(bssl::MakeConstSpan(out, KECCAK256_DIGEST_LENGTH)));

    // Drive the streaming EVP path one byte at a time to exercise the
    // partial-block buffering in Keccak256_Update, and confirm it matches the
    // one-shot. This goes through the same public entry point external callers
    // use (the streaming Keccak256_* primitives are not exported directly).
    bssl::ScopedEVP_MD_CTX ctx;
    ASSERT_TRUE(EVP_DigestInit_ex(ctx.get(), EVP_keccak256(), nullptr));
    for (size_t i = 0; i < byte_len; i++) {
      ASSERT_TRUE(EVP_DigestUpdate(ctx.get(), &msg[i], 1));
    }
    unsigned out_len = 0;
    OPENSSL_memset(out, 0, sizeof(out));
    ASSERT_TRUE(EVP_DigestFinal_ex(ctx.get(), out, &out_len));
    ASSERT_EQ(static_cast<unsigned>(KECCAK256_DIGEST_LENGTH), out_len);
    EXPECT_EQ(EncodeHex(bssl::MakeConstSpan(md)),
              EncodeHex(bssl::MakeConstSpan(out, KECCAK256_DIGEST_LENGTH)));
  };
  FileTestGTest("crypto/keccak/testvectors/KECCAK256ShortMsg.txt", run);
  FileTestGTest("crypto/keccak/testvectors/KECCAK256LongMsg.txt", run);
}
