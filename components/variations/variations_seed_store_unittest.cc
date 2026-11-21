/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/variations/variations_seed_store.h"

#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "base/files/file_path.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/strcat.h"
#include "base/strings/string_view_util.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "components/prefs/testing_pref_service.h"
#include "components/variations/metrics.h"
#include "components/variations/pref_names.h"
#include "components/variations/proto/variations_seed.pb.h"
#include "components/variations/variations_safe_seed_store.h"
#include "components/version_info/channel.h"
#include "crypto/hash.h"
#include "crypto/keypair.h"
#include "crypto/sign.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/boringssl/src/include/openssl/mldsa.h"

namespace variations {

namespace {

// Signs `seed_bytes` as upstream's hybrid seed signature format expects.
std::vector<uint8_t> SignHybrid(const MLDSA44_private_key& mldsa_priv,
                                const crypto::keypair::PrivateKey& ecdsa_priv,
                                const std::string& seed_bytes) {
  constexpr std::string_view kPrefix = "CompositeAlgorithmSignatures2025";
  constexpr std::string_view kLabel = "COMPSIG-MLDSA44-ECDSA-P256-SHA256";
  const auto seed_hash = crypto::hash::Sha256(seed_bytes);
  const std::string m_prime =
      base::StrCat({kPrefix, kLabel, std::string_view("\0", 1),
                    base::as_string_view(seed_hash)});
  const base::span<const uint8_t> m_prime_bytes = base::as_byte_span(m_prime);

  std::vector<uint8_t> signature(MLDSA44_SIGNATURE_BYTES);
  CHECK(MLDSA44_sign(signature.data(), &mldsa_priv, m_prime_bytes.data(),
                     m_prime_bytes.size(), base::as_byte_span(kLabel).data(),
                     kLabel.size()));
  std::vector<uint8_t> ecdsa_signature = crypto::sign::Sign(
      crypto::sign::SignatureKind::ECDSA_SHA256, ecdsa_priv, m_prime_bytes);
  signature.insert(signature.end(), ecdsa_signature.begin(),
                   ecdsa_signature.end());
  return signature;
}

// A hybrid key pinned as key ID 1 for the test's lifetime.
class PinnedHybridKey {
 public:
  PinnedHybridKey()
      : mldsa_priv_(std::make_unique<MLDSA44_private_key>()),
        ecdsa_priv_(crypto::keypair::PrivateKey::GenerateEcP256()) {
    std::array<uint8_t, MLDSA44_PUBLIC_KEY_BYTES> mldsa_pub = {};
    uint8_t mldsa_seed[MLDSA_SEED_BYTES];
    CHECK(
        MLDSA44_generate_key(mldsa_pub.data(), mldsa_seed, mldsa_priv_.get()));
    override_ = VariationsSeedStore::OverridePinnedHybridKeyForTesting(
        /*key_id=*/1, mldsa_pub,
        crypto::keypair::PublicKey::FromPrivateKey(ecdsa_priv_)
            .ToUncompressedX962Point());
  }

  // A valid `X-Seed-Signature` for `seed_bytes` under the pinned key.
  std::string Sign(const std::string& seed_bytes) const {
    return base::StrCat({"1:", base::Base64Encode(SignHybrid(
                                   *mldsa_priv_, ecdsa_priv_, seed_bytes))});
  }

 private:
  std::unique_ptr<MLDSA44_private_key> mldsa_priv_;
  crypto::keypair::PrivateKey ecdsa_priv_;
  base::ScopedClosureRunner override_;
};

}  // namespace

TEST(BraveVariationsSeedStoreTest, RejectsHybridSeedSignatures) {
  const PinnedHybridKey key;
  const std::string seed_bytes = "seed";
  EXPECT_EQ(VerifySignatureResult::kInvalidSeedSignature,
            VariationsSeedStore::VerifySeedSignatureForTesting(
                seed_bytes, key.Sign(seed_bytes)));
}

// Upstream stores a hybrid-signed seed and then distrusts the legacy key, which
// would reject every seed Brave's server signs.
TEST(BraveVariationsSeedStoreTest, RejectedHybridSeedKeepsLegacyKeyTrusted) {
  base::test::TaskEnvironment task_environment;
  TestingPrefServiceSimple local_state;
  VariationsSeedStore::RegisterPrefs(local_state.registry());
  VariationsSeedStore seed_store(
      &local_state, /*initial_seed=*/nullptr,
      /*signature_verification_enabled_on_load=*/true,
      /*signature_verification_enabled_on_receive=*/true,
      std::make_unique<VariationsSafeSeedStore>(&local_state, base::FilePath(),
                                                version_info::Channel::UNKNOWN,
                                                /*entropy_providers=*/nullptr),
      version_info::Channel::UNKNOWN, base::FilePath());

  const PinnedHybridKey key;
  VariationsSeed seed;
  seed.set_serial_number("1");
  const std::string seed_bytes = seed.SerializeAsString();

  base::test::TestFuture<bool, VariationsSeed> stored;
  seed_store.StoreSeedData(stored.GetCallback(), seed_bytes,
                           key.Sign(seed_bytes), /*country_code=*/"us",
                           /*geo_level1=*/"", base::Time::Now(),
                           /*is_delta_compressed=*/false,
                           /*is_gzip_compressed=*/false,
                           /*require_synchronous=*/true);
  EXPECT_FALSE(stored.Get<0>());
  EXPECT_FALSE(
      local_state.GetBoolean(prefs::kVariationsLegacySeedKeyDistrusted));
}

}  // namespace variations
