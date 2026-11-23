// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

#include "test/test_fixture.h"

#include <openssl/core_names.h>
#include <openssl/evp.h>
#include <openssl/params.h>

#include <algorithm>
#include <string>
#include <vector>

#include <cctype>

namespace awslc_provider_test {
namespace {

// One row per registered algorithm.
struct DigestSpec {
  const char *name;  // what we register it as
  size_t digest_size;
  size_t block_size;
  int xof;           // 1 only for an extendable-output function
  int algid_absent;  // 1 when the AlgorithmIdentifier omits its parameters
  const char *input;
  const char *expected_hex;  // FIPS 180-4
};

constexpr DigestSpec kDigests[] = {
    {"SHA2-224",
     28,
     64,
     0,
     1,
     "abc",
     "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7"},
    {"SHA2-256",
     32,
     64,
     0,
     1,
     "abc",
     "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
    {"SHA2-384",
     48,
     128,
     0,
     1,
     "abc",
     "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed"
     "8086072ba1e7cc2358baeca134c825a7"},
    {"SHA2-512",
     64,
     128,
     0,
     1,
     "abc",
     "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
     "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"},
    {"SHA2-512/224",
     28,
     128,
     0,
     1,
     "abc",
     "4634270f707b6a54daae7530460842e20e37ed265ceee9a43e8924aa"},
    {"SHA2-512/256",
     32,
     128,
     0,
     1,
     "abc",
     "53048e2681941ef99b2e29b76b4c7dabe4c2d0c634fc6d46e0e2f13107e7af23"},
};

// Renders |len| bytes as lowercase hex so a failure names the actual digest rather
// than reporting that two byte arrays differ.
std::string ToHex(const uint8_t *bytes, size_t len) {
  static const char kHex[] = "0123456789abcdef";
  std::string out;
  out.reserve(len * 2);
  for (size_t i = 0; i < len; i++) {
    out.push_back(kHex[bytes[i] >> 4]);
    out.push_back(kHex[bytes[i] & 0x0f]);
  }
  return out;
}

// Fetches require the provider rather than preferring it, so a silent fallthrough
// to the default provider cannot pass as success.
class Sha2Test : public ProviderTest,
                 public ::testing::WithParamInterface<DigestSpec> {
 protected:
  MdPtr FetchRequired() {
    return MdPtr(EVP_MD_fetch(libctx(), GetParam().name, kRequireAwslc));
  }
};

// ALGID_ABSENT is the one with teeth: it changes the DER OpenSSL emits for this
// digest inside PKI structures, so it must match the default provider's value.
TEST_P(Sha2Test, ReportsExpectedParams) {
  MdPtr md = FetchRequired();
  ASSERT_TRUE(md);

  EXPECT_EQ(static_cast<int>(GetParam().digest_size),
            EVP_MD_get_size(md.get()));
  EXPECT_EQ(static_cast<int>(GetParam().block_size),
            EVP_MD_get_block_size(md.get()));
  EXPECT_EQ(GetParam().xof, EVP_MD_xof(md.get()));

  int algid_absent = -1;
  OSSL_PARAM params[] = {
      OSSL_PARAM_construct_int(OSSL_DIGEST_PARAM_ALGID_ABSENT, &algid_absent),
      OSSL_PARAM_construct_end()};
  ASSERT_TRUE(EVP_MD_get_params(md.get(), params));
  EXPECT_EQ(GetParam().algid_absent, algid_absent);
}

// Known-answer coverage through the one-shot, streaming, and DUPCTX paths. The
// empty destination passed to EVP_MD_CTX_copy_ex has no digest method, so OpenSSL
// uses DUPCTX to allocate and copy its provider state.
TEST_P(Sha2Test, ComputesKnownAnswerAndDuplicatesIntoEmptyDestination) {
  MdPtr md = FetchRequired();
  ASSERT_TRUE(md);
  const std::string expected = GetParam().expected_hex;
  const std::string input = GetParam().input;

  // One shot.
  uint8_t out[EVP_MAX_MD_SIZE];
  unsigned out_len = 0;
  ASSERT_TRUE(
      EVP_Digest(input.data(), input.size(), out, &out_len, md.get(), nullptr));
  EXPECT_EQ(expected, ToHex(out, out_len));

  // Fed one byte at a time, with a zero-length update in front. A zero-length
  // update is legal and must be a no-op rather than an error or a state change.
  MdCtxPtr ctx(EVP_MD_CTX_new());
  ASSERT_TRUE(ctx);
  ASSERT_TRUE(EVP_DigestInit_ex(ctx.get(), md.get(), nullptr));
  ASSERT_TRUE(EVP_DigestUpdate(ctx.get(), input.data(), 0));
  for (char c : input) {
    ASSERT_TRUE(EVP_DigestUpdate(ctx.get(), &c, 1));
  }

  // Duplicated into an empty destination to exercise the DUPCTX slot.
  MdCtxPtr copy(EVP_MD_CTX_new());
  ASSERT_TRUE(copy);
  ASSERT_TRUE(EVP_MD_CTX_copy_ex(copy.get(), ctx.get()));

  uint8_t from_original[EVP_MAX_MD_SIZE], from_copy[EVP_MAX_MD_SIZE];
  unsigned original_len = 0, copy_len = 0;
  ASSERT_TRUE(EVP_DigestFinal_ex(ctx.get(), from_original, &original_len));
  ASSERT_TRUE(EVP_DigestFinal_ex(copy.get(), from_copy, &copy_len));

  EXPECT_EQ(expected, ToHex(from_original, original_len));
  EXPECT_EQ(expected, ToHex(from_copy, copy_len));
}

// OpenSSL 3.5.5 selects COPYCTX when both contexts hold the exact same digest
// method and that method provides the slot. Initializing each context with this
// one fetched EVP_MD satisfies the identity check while allocating separate
// provider contexts.
TEST_P(Sha2Test, CopyCtxOverwritesInitializedDestination) {
  MdPtr md = FetchRequired();
  ASSERT_TRUE(md);
  const std::string expected = GetParam().expected_hex;
  const std::string input = GetParam().input;
  ASSERT_FALSE(input.empty());
  const size_t split = input.size() / 2;

  MdCtxPtr source(EVP_MD_CTX_new());
  MdCtxPtr destination(EVP_MD_CTX_new());
  ASSERT_TRUE(source);
  ASSERT_TRUE(destination);
  ASSERT_TRUE(EVP_DigestInit_ex(source.get(), md.get(), nullptr));
  ASSERT_TRUE(EVP_DigestInit_ex(destination.get(), md.get(), nullptr));

  ASSERT_TRUE(EVP_DigestUpdate(source.get(), input.data(), split));
  constexpr char kUnrelatedState[] = "unrelated destination state";
  ASSERT_TRUE(EVP_DigestUpdate(destination.get(), kUnrelatedState,
                               sizeof(kUnrelatedState) - 1));

  ASSERT_TRUE(EVP_MD_CTX_copy_ex(destination.get(), source.get()));

  // Feed the suffix to each context separately. A no-op copy leaves the
  // destination's unrelated state intact, while shared state consumes the
  // suffix twice; both failures are caught by the known answer below.
  ASSERT_TRUE(EVP_DigestUpdate(source.get(), input.data() + split,
                               input.size() - split));
  ASSERT_TRUE(EVP_DigestUpdate(destination.get(), input.data() + split,
                               input.size() - split));

  uint8_t from_source[EVP_MAX_MD_SIZE], from_destination[EVP_MAX_MD_SIZE];
  unsigned source_len = 0, destination_len = 0;
  ASSERT_TRUE(EVP_DigestFinal_ex(source.get(), from_source, &source_len));
  ASSERT_TRUE(EVP_DigestFinal_ex(destination.get(), from_destination,
                                 &destination_len));

  EXPECT_EQ(expected, ToHex(from_source, source_len));
  EXPECT_EQ(expected, ToHex(from_destination, destination_len));
}

// Names the cell after the algorithm, so a failure reads
// Sha2/Sha2Test.ReportsExpectedParams/SHA2_256 rather than an index.
std::string SpecName(const testing::TestParamInfo<DigestSpec> &info) {
  std::string name = info.param.name;
  for (char &c : name) {
    if (!isalnum(static_cast<unsigned char>(c))) {
      c = '_';
    }
  }
  return name;
}

INSTANTIATE_TEST_SUITE_P(Sha2, Sha2Test, testing::ValuesIn(kDigests), SpecName);

}  // namespace
}  // namespace awslc_provider_test
