// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX-License-Identifier: Apache-2.0 OR ISC

// The provider-wide inventory of advertised algorithms and their attributed
// public fetch paths.

#include "test/test_fixture.h"

#include <openssl/core_dispatch.h>
#include <openssl/evp.h>

#include <algorithm>
#include <cctype>
#include <iterator>
#include <string>
#include <vector>

namespace awslc_provider_test {
namespace {

struct ReachabilityCell {
  int operation;
  const char *names;
};

// One attributed test cell per registry row, carrying the row's names string
// verbatim. Duplicates stay duplicated so the comparison below detects both
// missing coverage and accidental registrations.
constexpr ReachabilityCell kReachabilityCells[] = {
    {OSSL_OP_DIGEST, "SHA2-224:SHA-224:SHA224:2.16.840.1.101.3.4.2.4"},
    {OSSL_OP_DIGEST, "SHA2-256:SHA-256:SHA256:2.16.840.1.101.3.4.2.1"},
    {OSSL_OP_DIGEST, "SHA2-384:SHA-384:SHA384:2.16.840.1.101.3.4.2.2"},
    {OSSL_OP_DIGEST, "SHA2-512:SHA-512:SHA512:2.16.840.1.101.3.4.2.3"},
    {OSSL_OP_DIGEST,
     "SHA2-512/224:SHA-512/224:SHA512-224:2.16.840.1.101.3.4.2.5"},
    {OSSL_OP_DIGEST,
     "SHA2-512/256:SHA-512/256:SHA512-256:2.16.840.1.101.3.4.2.6"},
};

std::vector<std::string> SplitNames(const std::string &names) {
  std::vector<std::string> out;
  size_t start = 0;
  for (size_t end = 0; (end = names.find(':', start)) != std::string::npos;
       start = end + 1) {
    out.push_back(names.substr(start, end - start));
  }
  out.push_back(names.substr(start));
  return out;
}

std::string ReachabilityKey(int operation, const std::string &names) {
  return std::to_string(operation) + " " + names;
}

TEST_F(ProviderTest, AdvertisedAlgorithmsMatchReachabilityCells) {
  std::vector<std::string> advertised;
  for (int operation = 1; operation <= OSSL_OP__HIGHEST; operation++) {
    int no_cache = 0;
    const OSSL_ALGORITHM *algorithms =
        OSSL_PROVIDER_query_operation(awslc(), operation, &no_cache);

    for (const OSSL_ALGORITHM *algorithm = algorithms;
         algorithm != nullptr && algorithm->algorithm_names != nullptr;
         algorithm++) {
      advertised.push_back(
          ReachabilityKey(operation, algorithm->algorithm_names));
    }

    OSSL_PROVIDER_unquery_operation(awslc(), operation, algorithms);
  }

  std::vector<std::string> covered;
  for (const ReachabilityCell &cell : kReachabilityCells) {
    covered.push_back(ReachabilityKey(cell.operation, cell.names));
  }

  std::sort(advertised.begin(), advertised.end());
  std::sort(covered.begin(), covered.end());
  std::vector<std::string> uncovered, unadvertised;
  std::set_difference(advertised.begin(), advertised.end(), covered.begin(),
                      covered.end(), std::back_inserter(uncovered));
  std::set_difference(covered.begin(), covered.end(), advertised.begin(),
                      advertised.end(), std::back_inserter(unadvertised));
  EXPECT_TRUE(uncovered.empty()) << "advertised but not in kReachabilityCells: "
                                 << ::testing::PrintToString(uncovered);
  EXPECT_TRUE(unadvertised.empty())
      << "in kReachabilityCells but not advertised: "
      << ::testing::PrintToString(unadvertised);
}

class ReachabilityTest
    : public ProviderTest,
      public ::testing::WithParamInterface<ReachabilityCell> {};

std::string OperationName(int operation) {
  switch (operation) {
    case OSSL_OP_DIGEST:
      return "Digest";
    case OSSL_OP_CIPHER:
      return "Cipher";
    case OSSL_OP_MAC:
      return "Mac";
    case OSSL_OP_KDF:
      return "Kdf";
    case OSSL_OP_RAND:
      return "Rand";
    case OSSL_OP_KEYMGMT:
      return "KeyMgmt";
    case OSSL_OP_KEYEXCH:
      return "KeyExch";
    case OSSL_OP_SIGNATURE:
      return "Signature";
    case OSSL_OP_ASYM_CIPHER:
      return "AsymCipher";
    case OSSL_OP_KEM:
      return "Kem";
    case OSSL_OP_SKEYMGMT:
      return "SKeyMgmt";
    case OSSL_OP_ENCODER:
      return "Encoder";
    case OSSL_OP_DECODER:
      return "Decoder";
    case OSSL_OP_STORE:
      return "Store";
    default:
      return "Op" + std::to_string(operation);
  }
}

// Implicit-fetch consumers resolve by NID short name, so a missing alias makes
// the algorithm invisible to them with no error anywhere.
TEST_P(ReachabilityTest, ResolvesUnderEveryAdvertisedName) {
  const ReachabilityCell &cell = GetParam();

  for (const std::string &name : SplitNames(cell.names)) {
    switch (cell.operation) {
      case OSSL_OP_DIGEST: {
        MdPtr md(EVP_MD_fetch(libctx(), name.c_str(), kRequireAwslc));
        ASSERT_TRUE(md) << "advertised name '" << name << "' did not resolve";
        EXPECT_STREQ(kProviderName,
                     OSSL_PROVIDER_get0_name(EVP_MD_get0_provider(md.get())));
        break;
      }
      default:
        FAIL() << OperationName(cell.operation)
               << " has no attributed reachability handler";
    }
  }
}

std::string ReachabilityName(
    const testing::TestParamInfo<ReachabilityCell> &info) {
  std::string name = OperationName(info.param.operation) + "_" +
                     SplitNames(info.param.names).front();
  for (char &c : name) {
    if (!isalnum(static_cast<unsigned char>(c))) {
      c = '_';
    }
  }
  return name;
}

INSTANTIATE_TEST_SUITE_P(Provider, ReachabilityTest,
                         testing::ValuesIn(kReachabilityCells),
                         ReachabilityName);

}  // namespace
}  // namespace awslc_provider_test
