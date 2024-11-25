// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/brave_farbling_service.h"

#include <memory>
#include <tuple>

#include "base/test/task_environment.h"
#include "base/token.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

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
        std::get<0>(c), &prng));
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
        std::get<0>(c), &prng));
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
        farbling_service()->MakePseudoRandomGeneratorForURL(url, &prng));
    EXPECT_FALSE(
        farbling_service()->MakePseudoRandomGeneratorForURL(url, &prng));
  }
}

TEST_F(BraveFarblingServiceTest, ShieldsDown) {
  const GURL url("http://a.com");
  brave_shields::SetBraveShieldsEnabled(settings_map_.get(), false, url);
  brave::FarblingPRNG prng;
  EXPECT_FALSE(farbling_service()->MakePseudoRandomGeneratorForURL(url, &prng));
}

TEST_F(BraveFarblingServiceTest, FingerprintingAllowed) {
  const GURL url("http://a.com");
  brave_shields::SetFingerprintingControlType(
      settings_map_.get(), brave_shields::ControlType::ALLOW, url);
  brave::FarblingPRNG prng;
  EXPECT_FALSE(farbling_service()->MakePseudoRandomGeneratorForURL(url, &prng));
}
