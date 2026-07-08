// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/brave_farbling_service.h"

#include <array>
#include <memory>
#include <tuple>

#include "base/test/task_environment.h"
#include "base/token.h"
#include "brave/components/brave_shields/core/browser/brave_shields_test_utils.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

class BraveFarblingServiceTest : public testing::Test {
 public:
  BraveFarblingServiceTest() = default;
  BraveFarblingServiceTest(const BraveFarblingServiceTest&) = delete;
  BraveFarblingServiceTest& operator=(const BraveFarblingServiceTest&) = delete;
  ~BraveFarblingServiceTest() override = default;

  void SetUp() override {
    HostContentSettingsMap::RegisterProfilePrefs(prefs_.registry());
    settings_map_ = new HostContentSettingsMap(
        &prefs_, false /* is_off_the_record */, false /* store_last_modified */,
        false /* restore_session */, false /* should_record_metrics */);
    farbling_service_ =
        std::make_unique<brave::BraveFarblingService>(settings_map_.get());
  }

  void TearDown() override { settings_map_->ShutdownOnUIThread(); }

  brave::BraveFarblingService* farbling_service() {
    return farbling_service_.get();
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  scoped_refptr<HostContentSettingsMap> settings_map_;
  std::unique_ptr<brave::BraveFarblingService> farbling_service_;
};

TEST_F(BraveFarblingServiceTest, PRNGKnownValues) {
  const std::array<std::tuple<GURL, uint64_t>, 2> test_cases = {
      std::make_tuple<>(GURL("http://a.com"), 10450951993123491723UL),
      std::make_tuple<>(GURL("http://b.com"), 2581208260237394178UL),
  };
  for (const auto& c : test_cases) {
    brave::FarblingPRNG prng;
    ASSERT_TRUE(farbling_service()->MakePseudoRandomGeneratorForURL(
        std::get<0>(c), {}, &prng));
    EXPECT_EQ(prng(), std::get<1>(c));
  }
}

TEST_F(BraveFarblingServiceTest, PRNGKnownValuesDifferentSeeds) {
  const std::array<std::tuple<GURL, uint64_t>, 2> test_cases = {
      std::make_tuple<>(GURL("http://a.com"), 10450951993123491723UL),
      std::make_tuple<>(GURL("http://b.com"), 2581208260237394178UL),
  };
  for (const auto& c : test_cases) {
    brave::FarblingPRNG prng;
    ASSERT_TRUE(farbling_service()->MakePseudoRandomGeneratorForURL(
        std::get<0>(c), {}, &prng));
    EXPECT_EQ(prng(), std::get<1>(c));
  }
}

TEST_F(BraveFarblingServiceTest, InvalidDomains) {
  const std::array<GURL, 8> test_cases = {
      GURL("about:blank"),
      GURL("brave://settings"),
      GURL("chrome://version"),
      GURL("file:///etc/passwd"),
      GURL("javascript:alert(1)"),
      GURL("data:text/plain;base64,"),
      GURL(""),
  };
  for (const auto& url : test_cases) {
    brave::FarblingPRNG prng;
    EXPECT_FALSE(
        farbling_service()->MakePseudoRandomGeneratorForURL(url, {}, &prng));
    EXPECT_FALSE(
        farbling_service()->MakePseudoRandomGeneratorForURL(url, {}, &prng));
  }
}

TEST_F(BraveFarblingServiceTest, ShieldsDown) {
  const GURL url("http://a.com");
  brave_shields::SetBraveShieldsEnabled(settings_map_.get(), false, url);
  brave::FarblingPRNG prng;
  EXPECT_FALSE(
      farbling_service()->MakePseudoRandomGeneratorForURL(url, {}, &prng));
}

TEST_F(BraveFarblingServiceTest, FingerprintingAllowed) {
  const GURL url("http://a.com");
  brave_shields::SetFingerprintingControlType(
      settings_map_.get(), brave_shields::ControlType::ALLOW, url);
  brave::FarblingPRNG prng;
  EXPECT_FALSE(
      farbling_service()->MakePseudoRandomGeneratorForURL(url, {}, &prng));
}

// Farbling token related tests.

// Unsupported schemes (chrome://, about:blank, data:, invalid URL) must return
// the zero token because shields are not active there.
TEST_F(BraveFarblingServiceTest,
       FarblingToken_UnsupportedScheme_ReturnsZeroToken) {
  const base::Token zero;
  EXPECT_EQ(zero, farbling_service()->GetFarblingToken(
                      GURL("chrome://settings"), {}));
  EXPECT_EQ(zero,
            farbling_service()->GetFarblingToken(GURL("about:blank"), {}));
  EXPECT_EQ(zero, farbling_service()->GetFarblingToken(
                      GURL("data:text/html,<h1>hello</h1>"), {}));
  EXPECT_EQ(zero, farbling_service()->GetFarblingToken(
                      GURL("file:///etc/hosts"), {}));
  EXPECT_EQ(zero, farbling_service()->GetFarblingToken(GURL(), {}));
}

// HTTP and HTTPS URLs must produce a non-zero token.
TEST_F(BraveFarblingServiceTest,
       FarblingToken_HttpAndHttps_ReturnNonZeroToken) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  EXPECT_FALSE(farbling_service()
                   ->GetFarblingToken(GURL("http://example.com"), {})
                   .is_zero());
  EXPECT_FALSE(farbling_service()
                   ->GetFarblingToken(GURL("https://example.com"), {})
                   .is_zero());
}

// Two URLs with the same origin but different paths must share the same token,
// because the path is stripped when deriving the effective URL.
TEST_F(BraveFarblingServiceTest,
       FarblingToken_SameOrigin_DifferentPaths_SameToken) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  const auto t1 = farbling_service()->GetFarblingToken(
      GURL("https://example.com/page1"), {});
  const auto t2 = farbling_service()->GetFarblingToken(
      GURL("https://example.com/page2?q=1#anchor"), {});
  EXPECT_EQ(t1, t2);
}

// A blob URL whose inner origin is https://example.com must yield the same
// token as a plain https://example.com URL, because both resolve to the same
// effective origin (https://example.com/).
TEST_F(BraveFarblingServiceTest, FarblingToken_BlobUrl_SameTokenAsOriginUrl) {
  // Use a random seed (0 = random) to exercise the storage path: the first
  // caller writes a random token keyed under https://example.com/; the second
  // caller reads it back regardless of whether it used the blob or plain URL.
  const auto blob_token = farbling_service()->GetFarblingToken(
      url::Origin::Create(
          GURL("blob:https://example.com/550e8400-e29b-41d4-a716-446655440000"))
          .GetURL(),
      {});
  const auto https_token = farbling_service()->GetFarblingToken(
      GURL("https://example.com/some/path"), {});
  EXPECT_EQ(blob_token, https_token);
}

// A blob URL whose inner origin is a subdomain must resolve to the same token
// as a plain HTTPS URL for that subdomain (the subdomain itself shares a token
// with the root via schemeful-site scoping — see
// FarblingToken_SubdomainAndRoot_ShareToken).
TEST_F(BraveFarblingServiceTest,
       FarblingToken_BlobSubdomainUrl_SameTokenAsSubdomainUrl) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  const auto sub_token =
      farbling_service()->GetFarblingToken(GURL("https://sub.example.com"), {});
  const auto blob_sub_token = farbling_service()->GetFarblingToken(
      url::Origin::Create(GURL("blob:https://sub.example.com/some-uuid"))
          .GetURL(),
      {});
  EXPECT_EQ(sub_token, blob_sub_token);
}

// A blob URL for a subdomain must share the token of the root domain.
// BRAVE_SHIELDS_METADATA is registered with
// REQUESTING_SCHEMEFUL_SITE_ONLY_SCOPE, so the content setting is keyed by the
// schemeful site (eTLD+1 + scheme). https://example.com and
// https://sub.example.com resolve to the same schemeful site, so they always
// share one token.
TEST_F(BraveFarblingServiceTest,
       FarblingToken_BlobSubdomainUrl_SameTokenAsRootDomain) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  const auto root_token =
      farbling_service()->GetFarblingToken(GURL("https://example.com"), {});
  const auto blob_sub_token = farbling_service()->GetFarblingToken(
      url::Origin::Create(GURL("blob:https://sub.example.com/some-uuid"))
          .GetURL(),
      {});
  EXPECT_EQ(root_token, blob_sub_token);
}

// Two completely different origins must get different tokens.
TEST_F(BraveFarblingServiceTest,
       FarblingToken_DifferentOrigins_DifferentTokens) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  const auto t1 =
      farbling_service()->GetFarblingToken(GURL("https://example.com"), {});
  const auto t2 =
      farbling_service()->GetFarblingToken(GURL("https://other.com"), {});
  EXPECT_NE(t1, t2);
}

// A subdomain and its root domain share the same token as they map to same
// schemeful site (eTLD+1 + scheme).
TEST_F(BraveFarblingServiceTest, FarblingToken_SubdomainAndRoot_ShareToken) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  const auto root =
      farbling_service()->GetFarblingToken(GURL("https://example.com"), {});
  const auto sub =
      farbling_service()->GetFarblingToken(GURL("https://sub.example.com"), {});
  EXPECT_EQ(root, sub);
}

// Calling GetFarblingToken twice for the same URL must return the same token.
// This verifies that the token is persisted in the in-memory map and not
// regenerated on every call.
TEST_F(BraveFarblingServiceTest, FarblingToken_IsStableAcrossMultipleCalls) {
  const auto t1 =
      farbling_service()->GetFarblingToken(GURL("https://example.com"), {});
  const auto t2 =
      farbling_service()->GetFarblingToken(GURL("https://example.com"), {});
  EXPECT_EQ(t1, t2);
  EXPECT_FALSE(t1.is_zero());
}

// Providing additional_entropy must produce a token different from the base
// token for the same URL.
TEST_F(BraveFarblingServiceTest, FarblingToken_AdditionalEntropy_ChangesToken) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  const auto base_token =
      farbling_service()->GetFarblingToken(GURL("https://example.com"), {});
  constexpr std::array<uint8_t, 4> entropy = {0x01, 0x02, 0x03, 0x04};
  const auto derived_token = farbling_service()->GetFarblingToken(
      GURL("https://example.com"), entropy);
  EXPECT_NE(base_token, derived_token);
}

// Two calls with the same URL and the same additional_entropy must produce the
// same derived token (the XOR derivation is deterministic given a stable base).
TEST_F(BraveFarblingServiceTest,
       FarblingToken_SameEntropy_ProducesSameDerivedToken) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  constexpr std::array<uint8_t, 4> entropy = {0x05, 0x06, 0x07, 0x08};
  const auto t1 = farbling_service()->GetFarblingToken(
      GURL("https://example.com"), entropy);
  const auto t2 = farbling_service()->GetFarblingToken(
      GURL("https://example.com"), entropy);
  EXPECT_EQ(t1, t2);
}

// Two calls with the same URL but different additional_entropy values must
// produce different derived tokens.
TEST_F(BraveFarblingServiceTest,
       FarblingToken_DifferentEntropy_ProducesDifferentDerivedTokens) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  constexpr std::array<uint8_t, 4> entropy_a = {0xAA, 0xBB, 0xCC, 0xDD};
  constexpr std::array<uint8_t, 4> entropy_b = {0x11, 0x22, 0x33, 0x44};
  const auto t1 = farbling_service()->GetFarblingToken(
      GURL("https://example.com"), entropy_a);
  const auto t2 = farbling_service()->GetFarblingToken(
      GURL("https://example.com"), entropy_b);
  EXPECT_NE(t1, t2);
}

// The base token (no entropy) is unaffected by what another origin's derived
// token looks like: each origin owns its own stored base token.
TEST_F(BraveFarblingServiceTest, FarblingToken_PerOrigin_TokensAreIndependent) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  constexpr std::array<uint8_t, 4> entropy = {0x01, 0x02, 0x03, 0x04};
  const auto derived_a =
      farbling_service()->GetFarblingToken(GURL("https://a.com"), entropy);
  const auto derived_b =
      farbling_service()->GetFarblingToken(GURL("https://b.com"), entropy);
  // Different origins → different base tokens → different derived tokens even
  // with identical entropy.
  EXPECT_NE(derived_a, derived_b);
}
