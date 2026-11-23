// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

// The back side enforces output bounds that AWS-LC's size-less SHA*_Final
// functions cannot enforce themselves. OpenSSL's EVP layer always supplies the
// advertised digest size, so this guarantee must be tested directly here.

#include <gtest/gtest.h>

#include <stddef.h>

#include <vector>

#include "internal/backend/digests.h"

namespace {

const std::vector<unsigned char> kSha224Abc = {
    0x23, 0x09, 0x7d, 0x22, 0x34, 0x05, 0xd8, 0x22, 0x86, 0x42,
    0xa4, 0x77, 0xbd, 0xa2, 0x55, 0xb3, 0x2a, 0xad, 0xbc, 0xe4,
    0xbd, 0xa0, 0xb3, 0xf7, 0xe3, 0x6c, 0x9d, 0xa7};

const std::vector<unsigned char> kSha256Abc = {
    0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40,
    0xde, 0x5d, 0xae, 0x22, 0x23, 0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17,
    0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};

const std::vector<unsigned char> kSha384Abc = {
    0xcb, 0x00, 0x75, 0x3f, 0x45, 0xa3, 0x5e, 0x8b, 0xb5, 0xa0, 0x3d, 0x69,
    0x9a, 0xc6, 0x50, 0x07, 0x27, 0x2c, 0x32, 0xab, 0x0e, 0xde, 0xd1, 0x63,
    0x1a, 0x8b, 0x60, 0x5a, 0x43, 0xff, 0x5b, 0xed, 0x80, 0x86, 0x07, 0x2b,
    0xa1, 0xe7, 0xcc, 0x23, 0x58, 0xba, 0xec, 0xa1, 0x34, 0xc8, 0x25, 0xa7};

const std::vector<unsigned char> kSha512Abc = {
    0xdd, 0xaf, 0x35, 0xa1, 0x93, 0x61, 0x7a, 0xba, 0xcc, 0x41, 0x73,
    0x49, 0xae, 0x20, 0x41, 0x31, 0x12, 0xe6, 0xfa, 0x4e, 0x89, 0xa9,
    0x7e, 0xa2, 0x0a, 0x9e, 0xee, 0xe6, 0x4b, 0x55, 0xd3, 0x9a, 0x21,
    0x92, 0x99, 0x2a, 0x27, 0x4f, 0xc1, 0xa8, 0x36, 0xba, 0x3c, 0x23,
    0xa3, 0xfe, 0xeb, 0xbd, 0x45, 0x4d, 0x44, 0x23, 0x64, 0x3c, 0xe8,
    0x0e, 0x2a, 0x9a, 0xc9, 0x4f, 0xa5, 0x4c, 0xa4, 0x9f};

const std::vector<unsigned char> kSha512_224Abc = {
    0x46, 0x34, 0x27, 0x0f, 0x70, 0x7b, 0x6a, 0x54, 0xda, 0xae,
    0x75, 0x30, 0x46, 0x08, 0x42, 0xe2, 0x0e, 0x37, 0xed, 0x26,
    0x5c, 0xee, 0xe9, 0xa4, 0x3e, 0x89, 0x24, 0xaa};

const std::vector<unsigned char> kSha512_256Abc = {
    0x53, 0x04, 0x8e, 0x26, 0x81, 0x94, 0x1e, 0xf9, 0x9b, 0x2e, 0x29,
    0xb7, 0x6b, 0x4c, 0x7d, 0xab, 0xe4, 0xc2, 0xd0, 0xc6, 0x34, 0xfc,
    0x6d, 0x46, 0xe0, 0xe2, 0xf1, 0x31, 0x07, 0xe7, 0xaf, 0x23};

// One row per algorithm the back side wraps.
struct BackendDigest {
  const char *name;
  size_t (*ctx_size)(void);
  size_t (*digest_size)(void);
  int (*init)(void *ctx);
  int (*update)(void *ctx, const void *data, size_t len);
  int (*final)(void *ctx, unsigned char *out, size_t out_size);
  const std::vector<unsigned char> *expected;
};

const BackendDigest kBackendDigests[] = {
    {"SHA-224", awslc_prov_sha224_ctx_size, awslc_prov_sha224_digest_size,
     awslc_prov_sha224_init, awslc_prov_sha224_update,
     awslc_prov_sha224_final, &kSha224Abc},
    {"SHA-256", awslc_prov_sha256_ctx_size, awslc_prov_sha256_digest_size,
     awslc_prov_sha256_init, awslc_prov_sha256_update,
     awslc_prov_sha256_final, &kSha256Abc},
    {"SHA-384", awslc_prov_sha384_ctx_size, awslc_prov_sha384_digest_size,
     awslc_prov_sha384_init, awslc_prov_sha384_update,
     awslc_prov_sha384_final, &kSha384Abc},
    {"SHA-512", awslc_prov_sha512_ctx_size, awslc_prov_sha512_digest_size,
     awslc_prov_sha512_init, awslc_prov_sha512_update,
     awslc_prov_sha512_final, &kSha512Abc},
    {"SHA-512/224", awslc_prov_sha512_224_ctx_size,
     awslc_prov_sha512_224_digest_size, awslc_prov_sha512_224_init,
     awslc_prov_sha512_224_update, awslc_prov_sha512_224_final,
     &kSha512_224Abc},
    {"SHA-512/256", awslc_prov_sha512_256_ctx_size,
     awslc_prov_sha512_256_digest_size, awslc_prov_sha512_256_init,
     awslc_prov_sha512_256_update, awslc_prov_sha512_256_final,
     &kSha512_256Abc},
};

class BackendDigestTest : public testing::TestWithParam<BackendDigest> {};

TEST_P(BackendDigestTest, RejectsUndersizedOutputWithoutWriting) {
  const BackendDigest &d = GetParam();
  const unsigned char kCanary = 0xa5;
  const char kInput[] = "abc";
  std::vector<unsigned char> ctx(d.ctx_size(), 0);
  std::vector<unsigned char> output(d.digest_size(), kCanary);

  ASSERT_FALSE(ctx.empty());
  ASSERT_FALSE(output.empty());
  ASSERT_TRUE(d.init(ctx.data()));
  ASSERT_TRUE(d.update(ctx.data(), kInput, sizeof(kInput) - 1));

  // The allocation is deliberately large enough to keep a broken implementation
  // from causing undefined behavior. Only the reported size is short, and the
  // canary proves the size-less AWS-LC final function was never called.
  ASSERT_FALSE(d.final(ctx.data(), output.data(), output.size() - 1));
  for (unsigned char byte : output) {
    EXPECT_EQ(kCanary, byte);
  }

  // Rejection happens before AWS-LC sees the context, so the same operation can
  // still be finalized into a correctly sized buffer.
  ASSERT_TRUE(d.final(ctx.data(), output.data(), output.size()));
  EXPECT_EQ(*d.expected, output);
}

INSTANTIATE_TEST_SUITE_P(Digests, BackendDigestTest,
                         testing::ValuesIn(kBackendDigests),
                         [](const testing::TestParamInfo<BackendDigest> &info) {
                           std::string name = info.param.name;
                           for (char &c : name) {
                             if (c == '-' || c == '/') {
                               c = '_';
                             }
                           }
                           return name;
                         });

}  // namespace
