/*  Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/metrics/histogram_tester.h"
#include "brave/components/brave_shields/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/features.h"
#include "build/build_config.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/features.h"
#include "testing/gtest/include/gtest/gtest.h"

using brave_shields::ControlType;
using brave_shields::ControlTypeFromString;
using brave_shields::ControlTypeToString;
using brave_shields::DomainBlockingType;
using brave_shields::GetPatternFromURL;
using brave_shields::features::kBraveDomainBlock;

class BraveShieldsUtilTest : public testing::Test {
 public:
  BraveShieldsUtilTest() = default;
  BraveShieldsUtilTest(const BraveShieldsUtilTest&) = delete;
  BraveShieldsUtilTest& operator=(const BraveShieldsUtilTest&) = delete;
  ~BraveShieldsUtilTest() override = default;

  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    histogram_tester_ = std::make_unique<base::HistogramTester>();
  }

  TestingProfile* profile() { return profile_.get(); }

  void ExpectDomainBlockingType(const GURL& url,
                                DomainBlockingType domain_blocking_type) {
    auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
    auto setting = brave_shields::GetDomainBlockingType(map, url);
    EXPECT_EQ(domain_blocking_type, setting);
  }

 protected:
  std::unique_ptr<base::HistogramTester> histogram_tester_;

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
};

class BraveShieldsUtilDomainBlockFeatureTest : public BraveShieldsUtilTest {
 public:
  BraveShieldsUtilDomainBlockFeatureTest() {
    feature_list_.InitAndDisableFeature(kBraveDomainBlock);
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(BraveShieldsUtilTest, GetPatternFromURL) {
  // wildcard
  auto pattern = GetPatternFromURL(GURL());
  EXPECT_EQ(ContentSettingsPattern::Wildcard(), pattern);

  // scheme is a wildcard, should match any scheme
  pattern = GetPatternFromURL(GURL("http://brave.com"));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com/path1")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com/path2")));
  EXPECT_TRUE(pattern.Matches(GURL("https://brave.com")));
  EXPECT_TRUE(pattern.Matches(GURL("ftp://brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("http://subdomain.brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("http://brave2.com")));

  // path is a wildcard
  pattern = GetPatternFromURL(GURL("http://brave.com/path1"));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com/path1")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com/path2")));
  EXPECT_FALSE(pattern.Matches(GURL("http://subdomain.brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("http://brave2.com")));

  // port is a wildcard
  pattern = GetPatternFromURL(GURL("http://brave.com:8080"));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com:8080")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com:8080/path1")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com:8080/path2")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com:5555")));
  EXPECT_TRUE(pattern.Matches(GURL("https://brave.com")));
  EXPECT_TRUE(pattern.Matches(GURL("https://brave.com:8080")));
  EXPECT_FALSE(pattern.Matches(GURL("http://subdomain.brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("http://brave2.com")));

  // with implied port
  pattern = GetPatternFromURL(GURL("https://brianbondy.com"));
  EXPECT_EQ(pattern.ToString(), "brianbondy.com");
  pattern = GetPatternFromURL(GURL("http://brianbondy.com"));
  EXPECT_EQ(pattern.ToString(), "brianbondy.com");
  // with specified port
  pattern = GetPatternFromURL(GURL("http://brianbondy.com:8080"));
  EXPECT_EQ(pattern.ToString(), "brianbondy.com");
}

TEST_F(BraveShieldsUtilTest, ControlTypeToString) {
  EXPECT_EQ("block", ControlTypeToString(ControlType::BLOCK));
  EXPECT_EQ("allow", ControlTypeToString(ControlType::ALLOW));
  EXPECT_EQ("block_third_party",
            ControlTypeToString(ControlType::BLOCK_THIRD_PARTY));
}

TEST_F(BraveShieldsUtilTest, ControlTypeFromString) {
  EXPECT_EQ(ControlType::BLOCK, ControlTypeFromString("block"));
  EXPECT_EQ(ControlType::ALLOW, ControlTypeFromString("allow"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY,
            ControlTypeFromString("block_third_party"));
}

/* BRAVE_SHIELDS CONTROL */
TEST_F(BraveShieldsUtilTest, SetBraveShieldsEnabled_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  brave_shields::SetBraveShieldsEnabled(map, true, GURL("http://brave.com"));
  // setting should apply to origin
  auto setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                        ContentSettingsType::BRAVE_SHIELDS);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should apply to different scheme
  setting = map->GetContentSetting(GURL("https://brave.com"), GURL(),
                                   ContentSettingsType::BRAVE_SHIELDS);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should not apply to default
  setting = map->GetContentSetting(GURL(), GURL(),
                                   ContentSettingsType::BRAVE_SHIELDS);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
}

TEST_F(BraveShieldsUtilTest, SetBraveShieldsEnabled_IsNotHttpHttps) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  auto setting =
      brave_shields::GetBraveShieldsEnabled(map, GURL("chrome://preferences"));
  EXPECT_EQ(false, setting);
  brave_shields::SetBraveShieldsEnabled(map, ControlType::ALLOW,
                                        GURL("chrome://preferences"));
  setting =
      brave_shields::GetBraveShieldsEnabled(map, GURL("chrome://preferences"));
  EXPECT_EQ(false, setting);

  setting = brave_shields::GetBraveShieldsEnabled(map, GURL("about:blank"));
  EXPECT_EQ(false, setting);
  brave_shields::SetBraveShieldsEnabled(map, ControlType::ALLOW,
                                        GURL("about:blank"));
  setting = brave_shields::GetBraveShieldsEnabled(map, GURL("about:blank"));
  EXPECT_EQ(false, setting);
}

TEST_F(BraveShieldsUtilTest, GetBraveShieldsEnabled_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetBraveShieldsEnabled(map, GURL());
  EXPECT_EQ(true, setting);
  setting =
      brave_shields::GetBraveShieldsEnabled(map, GURL("http://brave.com"));
  EXPECT_EQ(true, setting);
  setting =
      brave_shields::GetBraveShieldsEnabled(map, GURL("https://brave.com"));
  EXPECT_EQ(true, setting);

  /* BLOCK */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_SHIELDS,
      CONTENT_SETTING_BLOCK);
  setting =
      brave_shields::GetBraveShieldsEnabled(map, GURL("http://brave.com/*"));
  EXPECT_EQ(false, setting);
  // https in unchanged
  setting =
      brave_shields::GetBraveShieldsEnabled(map, GURL("https://brave.com"));
  EXPECT_EQ(true, setting);
  // default is unchanged
  setting = brave_shields::GetBraveShieldsEnabled(map, GURL());
  EXPECT_EQ(true, setting);
}

TEST_F(BraveShieldsUtilTest, GetBraveShieldsEnabled_IsNotHttpHttps) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  auto setting =
      brave_shields::GetBraveShieldsEnabled(map, GURL("chrome://preferences"));
  EXPECT_EQ(false, setting);

  setting = brave_shields::GetBraveShieldsEnabled(map, GURL("about:blank"));
  EXPECT_EQ(false, setting);
}

/* AD CONTROL */
TEST_F(BraveShieldsUtilTest, SetAdControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  // settings should be default
  auto setting =
      map->GetContentSetting(GURL(), GURL(), ContentSettingsType::BRAVE_ADS);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   ContentSettingsType::BRAVE_ADS);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);

  /* ALLOW */
  brave_shields::SetAdControlType(map, ControlType::ALLOW, GURL());
  setting =
      map->GetContentSetting(GURL(), GURL(), ContentSettingsType::BRAVE_ADS);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // override should apply to all origins
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   ContentSettingsType::BRAVE_ADS);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  /* BLOCK */
  brave_shields::SetAdControlType(map, ControlType::BLOCK, GURL());
  setting =
      map->GetContentSetting(GURL(), GURL(), ContentSettingsType::BRAVE_ADS);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  // override should apply to all origins
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   ContentSettingsType::BRAVE_ADS);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
}

TEST_F(BraveShieldsUtilTest, SetAdControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  brave_shields::SetAdControlType(map, ControlType::ALLOW,
                                  GURL("http://brave.com"));
  // setting should apply to origin
  auto setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                        ContentSettingsType::BRAVE_ADS);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should also apply to different scheme
  setting = map->GetContentSetting(GURL("https://brave.com"), GURL(),
                                   ContentSettingsType::BRAVE_ADS);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should not apply to default
  setting =
      map->GetContentSetting(GURL(), GURL(), ContentSettingsType::BRAVE_ADS);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
}

TEST_F(BraveShieldsUtilTest, GetAdControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetAdControlType(map, GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_ADS, CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetAdControlType(map, GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);

  /* BLOCK */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_ADS, CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetAdControlType(map, GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);
}

TEST_F(BraveShieldsUtilTest, GetAdControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetAdControlType(map, GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting = brave_shields::GetAdControlType(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting = brave_shields::GetAdControlType(map, GURL("https://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_ADS,
      CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetAdControlType(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);

  // https in unchanged
  setting = brave_shields::GetAdControlType(map, GURL("https://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  // default is unchanged
  setting = brave_shields::GetAdControlType(map, GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);

  /* BLOCK */
  // change default to allow
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_ADS, CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetAdControlType(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting = brave_shields::GetAdControlType(map, GURL("https://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting = brave_shields::GetAdControlType(map, GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);

  // set override to block
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_ADS,
      CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetAdControlType(map, GURL("http://brave.com/*"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  // https in unchanged
  setting = brave_shields::GetAdControlType(map, GURL("https://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  // default is unchanged
  setting = brave_shields::GetAdControlType(map, GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);
}

/* COOKIE CONTROL */
TEST_F(BraveShieldsUtilTest, SetCookieControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  // setting should be default to start with
  auto setting = map->GetContentSetting(GURL(), GURL(),
                                        ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   ContentSettingsType::BRAVE_COOKIES);
  setting = map->GetContentSetting(GURL("http://brave.com"),
                                   GURL("https://firstParty"),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);

  /* ALLOW */
  brave_shields::SetCookieControlType(map, ControlType::ALLOW, GURL());
  setting = map->GetContentSetting(GURL(), GURL(),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  // setting should apply to all urls
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  setting = map->GetContentSetting(GURL("http://brave.com"),
                                   GURL("https://firstParty"),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  /* BLOCK */
  brave_shields::SetCookieControlType(map, ControlType::BLOCK, GURL());
  setting = map->GetContentSetting(GURL(), GURL(),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  // setting should apply to all urls
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(GURL("http://brave.com"),
                                   GURL("https://firstParty"),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  /* BLOCK_THIRD_PARTY */
  brave_shields::SetCookieControlType(map, ControlType::BLOCK_THIRD_PARTY,
                                      GURL());
  setting = map->GetContentSetting(GURL(), GURL(),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should apply to all urls
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(GURL("http://brave.com"),
                                   GURL("https://firstParty"),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
}

TEST_F(BraveShieldsUtilTest, SetCookieControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  brave_shields::SetCookieControlType(map, ControlType::ALLOW,
                                      GURL("http://brave.com"));
  // override should apply to origin
  auto setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                        ContentSettingsType::BRAVE_COOKIES);
  setting = map->GetContentSetting(GURL("http://brave.com"),
                                   GURL("https://firstParty"),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // override should also apply to different scheme
  setting = map->GetContentSetting(GURL("https://brave.com"), GURL(),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  setting = map->GetContentSetting(GURL("https://brave.com"),
                                   GURL("https://firstParty"),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // override should not apply to default
  setting = map->GetContentSetting(GURL(), GURL(),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  // override should not apply to default
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   ContentSettingsType::BRAVE_COOKIES);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
}

TEST_F(BraveShieldsUtilTest, GetCookieControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetCookieControlType(map, GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
  setting = brave_shields::GetCookieControlType(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_COOKIES, CONTENT_SETTING_ALLOW);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      ContentSettingsType::BRAVE_COOKIES, CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetCookieControlType(map, GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting = brave_shields::GetCookieControlType(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);

  /* BLOCK */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_COOKIES, CONTENT_SETTING_BLOCK);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      ContentSettingsType::BRAVE_COOKIES, CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetCookieControlType(map, GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting = brave_shields::GetCookieControlType(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);

  /* BLOCK_THIRD_PARTY */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_COOKIES, CONTENT_SETTING_BLOCK);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      ContentSettingsType::BRAVE_COOKIES, CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetCookieControlType(map, GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
  setting = brave_shields::GetCookieControlType(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
}

TEST_F(BraveShieldsUtilTest, GetCookieControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting =
      brave_shields::GetCookieControlType(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_COOKIES,
      CONTENT_SETTING_ALLOW);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      ContentSettingsType::BRAVE_COOKIES, CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetCookieControlType(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting = brave_shields::GetCookieControlType(map, GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* BLOCK */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_COOKIES,
      CONTENT_SETTING_BLOCK);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      ContentSettingsType::BRAVE_COOKIES, CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetCookieControlType(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting = brave_shields::GetCookieControlType(map, GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* BLOCK_THIRD_PARTY */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::BRAVE_COOKIES,
      CONTENT_SETTING_BLOCK);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      ContentSettingsType::BRAVE_COOKIES, CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetCookieControlType(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
  setting = brave_shields::GetCookieControlType(map, GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
}

/* FINGERPRINTING CONTROL */
TEST_F(BraveShieldsUtilTest, SetFingerprintingControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  // setting should be default to start with
  auto type = brave_shields::GetFingerprintingControlType(map, GURL());
  EXPECT_EQ(ControlType::DEFAULT, type);
  type = brave_shields::GetFingerprintingControlType(map,
                                                     GURL("http://brave.com"));
  EXPECT_EQ(ControlType::DEFAULT, type);

  /* ALLOW */
  brave_shields::SetFingerprintingControlType(map, ControlType::ALLOW, GURL());
  type = brave_shields::GetFingerprintingControlType(map, GURL());
  EXPECT_EQ(ControlType::ALLOW, type);

  // setting should apply to all urls
  type = brave_shields::GetFingerprintingControlType(map,
                                                     GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, type);

  /* BLOCK */
  brave_shields::SetFingerprintingControlType(map, ControlType::BLOCK, GURL());
  type = brave_shields::GetFingerprintingControlType(map, GURL());
  EXPECT_EQ(ControlType::BLOCK, type);

  // setting should apply to all urls
  type = brave_shields::GetFingerprintingControlType(map,
                                                     GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, type);

  /* DEFAULT */
  brave_shields::SetFingerprintingControlType(map, ControlType::DEFAULT,
                                              GURL());
  type = brave_shields::GetFingerprintingControlType(map, GURL());
  EXPECT_EQ(ControlType::DEFAULT, type);

  // setting should apply to all urls
  type = brave_shields::GetFingerprintingControlType(map,
                                                     GURL("http://brave.com"));
  EXPECT_EQ(ControlType::DEFAULT, type);

  /* Global ALLOW and Site explicit DEFAULT */
  brave_shields::SetFingerprintingControlType(map, ControlType::ALLOW, GURL());
  brave_shields::SetFingerprintingControlType(map, ControlType::DEFAULT,
                                              GURL("http://brave.com"));
  // Site should have DEFAULT if it's explicitly set.
  type = brave_shields::GetFingerprintingControlType(map,
                                                     GURL("http://brave.com"));
  EXPECT_EQ(ControlType::DEFAULT, type);

  /* Global BLOCK and Site explicit DEFAULT */
  brave_shields::SetFingerprintingControlType(map, ControlType::BLOCK, GURL());
  // Site should have DEFAULT if it's explicitly set.
  type = brave_shields::GetFingerprintingControlType(map,
                                                     GURL("http://brave.com"));
  EXPECT_EQ(ControlType::DEFAULT, type);
}

TEST_F(BraveShieldsUtilTest, SetFingerprintingControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  brave_shields::SetFingerprintingControlType(map, ControlType::ALLOW,
                                              GURL("http://brave.com"));
  auto type = brave_shields::GetFingerprintingControlType(
      map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, type);
  // override should also apply to different scheme
  type = brave_shields::GetFingerprintingControlType(map,
                                                     GURL("https://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, type);

  brave_shields::SetFingerprintingControlType(map, ControlType::BLOCK,
                                              GURL("http://brave.com"));
  type = brave_shields::GetFingerprintingControlType(map,
                                                     GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, type);
  // override should also apply to different scheme
  type = brave_shields::GetFingerprintingControlType(map,
                                                     GURL("https://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, type);

  // override should not apply to default
  type = brave_shields::GetFingerprintingControlType(map, GURL());
  EXPECT_EQ(ControlType::DEFAULT, type);
}

/* HTTPSEVERYWHERE CONTROL */
TEST_F(BraveShieldsUtilTest, SetHTTPSEverywhereEnabled_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  // settings should be default
  auto setting = map->GetContentSetting(
      GURL(), GURL(), ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  setting = map->GetContentSetting(
      GURL("http://brave.com"), GURL(),
      ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);

  /* disabled */
  brave_shields::SetHTTPSEverywhereEnabled(map, false, GURL());
  setting = map->GetContentSetting(
      GURL(), GURL(), ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // override should apply to all origins
  setting = map->GetContentSetting(
      GURL("http://brave.com"), GURL(),
      ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  /* enabled */
  brave_shields::SetHTTPSEverywhereEnabled(map, true, GURL());
  setting = map->GetContentSetting(
      GURL(), GURL(), ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  // override should apply to all origins
  setting = map->GetContentSetting(
      GURL("http://brave.com"), GURL(),
      ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
}

TEST_F(BraveShieldsUtilTest, SetHTTPSEverywhereEnabled_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  brave_shields::SetHTTPSEverywhereEnabled(map, false,
                                           GURL("http://brave.com"));
  // setting should apply to origin
  auto setting = map->GetContentSetting(
      GURL("http://brave.com"), GURL(),
      ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should apply to different scheme
  setting = map->GetContentSetting(
      GURL("https://brave.com"), GURL(),
      ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should not apply to default
  setting = map->GetContentSetting(
      GURL(), GURL(), ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
}

TEST_F(BraveShieldsUtilTest, GetHTTPSEverywhereEnabled_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetHTTPSEverywhereEnabled(map, GURL());
  EXPECT_EQ(true, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
      CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetHTTPSEverywhereEnabled(map, GURL());
  EXPECT_EQ(false, setting);

  /* BLOCK */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
      CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetHTTPSEverywhereEnabled(map, GURL());
  EXPECT_EQ(true, setting);
}

TEST_F(BraveShieldsUtilTest, GetHTTPSEverywhereEnabled_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetHTTPSEverywhereEnabled(map, GURL());
  EXPECT_EQ(true, setting);
  setting =
      brave_shields::GetHTTPSEverywhereEnabled(map, GURL("http://brave.com"));
  EXPECT_EQ(true, setting);
  setting =
      brave_shields::GetHTTPSEverywhereEnabled(map, GURL("https://brave.com"));
  EXPECT_EQ(true, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
      CONTENT_SETTING_ALLOW);
  setting =
      brave_shields::GetHTTPSEverywhereEnabled(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);

  // https in unchanged
  setting =
      brave_shields::GetHTTPSEverywhereEnabled(map, GURL("https://brave.com"));
  EXPECT_EQ(true, setting);
  // default is unchanged
  setting = brave_shields::GetHTTPSEverywhereEnabled(map, GURL());
  EXPECT_EQ(true, setting);

  /* BLOCK */
  // change default to allow
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
      CONTENT_SETTING_ALLOW);
  setting =
      brave_shields::GetHTTPSEverywhereEnabled(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting =
      brave_shields::GetHTTPSEverywhereEnabled(map, GURL("https://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting = brave_shields::GetHTTPSEverywhereEnabled(map, GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);

  // set override to block
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(),
      ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
      CONTENT_SETTING_BLOCK);
  setting =
      brave_shields::GetHTTPSEverywhereEnabled(map, GURL("http://brave.com/*"));
  EXPECT_EQ(true, setting);
  // https in unchanged
  setting =
      brave_shields::GetHTTPSEverywhereEnabled(map, GURL("https://brave.com"));
  EXPECT_EQ(false, setting);
  // default is unchanged
  setting = brave_shields::GetHTTPSEverywhereEnabled(map, GURL());
  EXPECT_EQ(false, setting);
}

/* NOSCRIPT CONTROL */
TEST_F(BraveShieldsUtilTest, SetNoScriptControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  // settings should be default
  auto setting =
      map->GetContentSetting(GURL(), GURL(), ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  /* BLOCK */
  brave_shields::SetNoScriptControlType(map, ControlType::BLOCK, GURL());
  setting =
      map->GetContentSetting(GURL(), GURL(), ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  // override should apply to all origins
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  /* ALLOW */
  brave_shields::SetNoScriptControlType(map, ControlType::ALLOW, GURL());
  setting =
      map->GetContentSetting(GURL(), GURL(), ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // override should apply to all origins
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
}

TEST_F(BraveShieldsUtilTest, SetNoScriptControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  brave_shields::SetNoScriptControlType(map, ControlType::BLOCK,
                                        GURL("http://brave.com"));
  // setting should apply to origin
  auto setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                        ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  // setting should also apply to different scheme
  setting = map->GetContentSetting(GURL("https://brave.com"), GURL(),
                                   ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  // setting should not apply to default
  setting =
      map->GetContentSetting(GURL(), GURL(), ContentSettingsType::JAVASCRIPT);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
}

TEST_F(BraveShieldsUtilTest, GetNoScriptControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetNoScriptControlType(map, GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);

  /* BLOCK */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetNoScriptControlType(map, GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetNoScriptControlType(map, GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);
}

TEST_F(BraveShieldsUtilTest, GetNoScriptControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetNoScriptControlType(map, GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting =
      brave_shields::GetNoScriptControlType(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting =
      brave_shields::GetNoScriptControlType(map, GURL("https://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);

  /* BLOCK */
  // set override to block
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::JAVASCRIPT,
      CONTENT_SETTING_BLOCK);
  setting =
      brave_shields::GetNoScriptControlType(map, GURL("http://brave.com/*"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  // https in unchanged
  setting =
      brave_shields::GetNoScriptControlType(map, GURL("https://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  // default is unchanged
  setting = brave_shields::GetNoScriptControlType(map, GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);

  /* ALLOW */
  // change default to block
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      ContentSettingsType::JAVASCRIPT, CONTENT_SETTING_BLOCK);
  setting =
      brave_shields::GetNoScriptControlType(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting =
      brave_shields::GetNoScriptControlType(map, GURL("https://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting = brave_shields::GetNoScriptControlType(map, GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);

  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::JAVASCRIPT,
      CONTENT_SETTING_ALLOW);
  setting =
      brave_shields::GetNoScriptControlType(map, GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);

  // https in unchanged
  setting =
      brave_shields::GetNoScriptControlType(map, GURL("https://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  // default is unchanged
  setting = brave_shields::GetNoScriptControlType(map, GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);
}

// Should not do domain blocking if domain blocking feature is disabled
TEST_F(BraveShieldsUtilDomainBlockFeatureTest, GetDomainBlockingType) {
  ExpectDomainBlockingType(GURL("https://brave.com"),
                           DomainBlockingType::kNone);
}

// Should not do domain blocking if Brave Shields is down
TEST_F(BraveShieldsUtilTest, GetDomainBlockingType_ShieldsDown) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  GURL url = GURL("https://brave.com");
  brave_shields::SetBraveShieldsEnabled(map, false, url);
  ExpectDomainBlockingType(url, DomainBlockingType::kNone);
}

// Should not do domain blocking on non-HTTP(S) URLs
TEST_F(BraveShieldsUtilTest, GetDomainBlockingType_IsNotHttpHttps) {
  ExpectDomainBlockingType(GURL("chrome://preferences"),
                           DomainBlockingType::kNone);
  ExpectDomainBlockingType(GURL("about:blank"), DomainBlockingType::kNone);
}

// Should not do domain blocking unless ad blocking is "aggressive"
TEST_F(BraveShieldsUtilTest, GetDomainBlockingType_ControlTypes) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  const GURL url = GURL("https://brave.com");

  const struct TestCase {
    ControlType ad_control_type;
    ControlType cosmetic_filtering_control_type;
    DomainBlockingType expected_blocking_type;
  } kTestCases[] = {
      {ControlType::ALLOW, ControlType::ALLOW, DomainBlockingType::kNone},
      {ControlType::ALLOW, ControlType::BLOCK_THIRD_PARTY,
       DomainBlockingType::kNone},
      {ControlType::ALLOW, ControlType::BLOCK, DomainBlockingType::kNone},
      {ControlType::BLOCK, ControlType::ALLOW, DomainBlockingType::kNone},
      {ControlType::BLOCK, ControlType::BLOCK_THIRD_PARTY,
       DomainBlockingType::kNone},
      {ControlType::BLOCK, ControlType::BLOCK, DomainBlockingType::kAggressive},
  };

  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(testing::Message()
                 << static_cast<int>(test_case.ad_control_type)
                 << static_cast<int>(test_case.cosmetic_filtering_control_type)
                 << static_cast<int>(test_case.expected_blocking_type));
    brave_shields::SetAdControlType(map, test_case.ad_control_type, url);
    brave_shields::SetCosmeticFilteringControlType(
        map, test_case.cosmetic_filtering_control_type, url);
    ExpectDomainBlockingType(url, test_case.expected_blocking_type);
  }
}

TEST_F(BraveShieldsUtilTest, RecordAdBlockSetting) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  brave_shields::SetCosmeticFilteringControlType(map, ControlType::BLOCK,
                                                 GURL("https://brave.com"));
  // Should not report to histogram if not a global change
  histogram_tester_->ExpectTotalCount(brave_shields::kAdsSettingHistogramName,
                                      0);

  brave_shields::SetCosmeticFilteringControlType(map, ControlType::BLOCK,
                                                 GURL());
  histogram_tester_->ExpectBucketCount(brave_shields::kAdsSettingHistogramName,
                                       2, 1);

  brave_shields::SetCosmeticFilteringControlType(
      map, ControlType::BLOCK_THIRD_PARTY, GURL());
  histogram_tester_->ExpectBucketCount(brave_shields::kAdsSettingHistogramName,
                                       1, 1);

  brave_shields::SetCosmeticFilteringControlType(map, ControlType::ALLOW,
                                                 GURL());
  histogram_tester_->ExpectBucketCount(brave_shields::kAdsSettingHistogramName,
                                       0, 1);

  histogram_tester_->ExpectTotalCount(brave_shields::kAdsSettingHistogramName,
                                      3);
}

TEST_F(BraveShieldsUtilTest, RecordFingerprintBlockSetting) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  brave_shields::SetFingerprintingControlType(map, ControlType::BLOCK,
                                              GURL("https://brave.com"));
  // Should not report to histogram if not a global change
  histogram_tester_->ExpectTotalCount(
      brave_shields::kFingerprintSettingHistogramName, 0);

  brave_shields::SetFingerprintingControlType(map, ControlType::BLOCK, GURL());
  histogram_tester_->ExpectBucketCount(
      brave_shields::kFingerprintSettingHistogramName, 2, 1);

  brave_shields::SetFingerprintingControlType(map, ControlType::DEFAULT,
                                              GURL());
  histogram_tester_->ExpectBucketCount(
      brave_shields::kFingerprintSettingHistogramName, 1, 1);

  brave_shields::SetFingerprintingControlType(map, ControlType::ALLOW, GURL());
  histogram_tester_->ExpectBucketCount(
      brave_shields::kFingerprintSettingHistogramName, 0, 1);

  histogram_tester_->ExpectTotalCount(
      brave_shields::kFingerprintSettingHistogramName, 3);
}

class BraveShieldsUtilDomainBlock1PESFeatureTest : public BraveShieldsUtilTest {
 public:
  BraveShieldsUtilDomainBlock1PESFeatureTest() {
    feature_list_.InitWithFeatures(
        {brave_shields::features::kBraveDomainBlock1PES,
         net::features::kBraveFirstPartyEphemeralStorage},
        {});
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// Should do 1PES domain blocking if domain blocking feature is enabled.
TEST_F(BraveShieldsUtilDomainBlock1PESFeatureTest, GetDomainBlockingType) {
  ExpectDomainBlockingType(GURL("https://brave.com"),
                           DomainBlockingType::k1PES);
}

// Should not do domain blocking if Brave Shields is down
TEST_F(BraveShieldsUtilDomainBlock1PESFeatureTest,
       GetDomainBlockingType_ShieldsDown) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  GURL url = GURL("https://brave.com");
  brave_shields::SetBraveShieldsEnabled(map, false, url);
  ExpectDomainBlockingType(url, DomainBlockingType::kNone);
}

// Should not do domain blocking on non-HTTP(S) URLs
TEST_F(BraveShieldsUtilDomainBlock1PESFeatureTest,
       GetDomainBlockingType_IsNotHttpHttps) {
  ExpectDomainBlockingType(GURL("chrome://preferences"),
                           DomainBlockingType::kNone);
  ExpectDomainBlockingType(GURL("about:blank"), DomainBlockingType::kNone);
}

// Should do 1PES domain blocking in "standard" mode.
TEST_F(BraveShieldsUtilDomainBlock1PESFeatureTest,
       GetDomainBlockingType_ControlTypes) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  GURL url = GURL("https://brave.com");

  const struct TestCase {
    ControlType ad_control_type;
    ControlType cosmetic_filtering_control_type;
    DomainBlockingType expected_blocking_type;
  } kTestCases[] = {
      {ControlType::ALLOW, ControlType::ALLOW, DomainBlockingType::kNone},
      {ControlType::ALLOW, ControlType::BLOCK_THIRD_PARTY,
       DomainBlockingType::kNone},
      {ControlType::ALLOW, ControlType::BLOCK, DomainBlockingType::kNone},
      {ControlType::BLOCK, ControlType::ALLOW, DomainBlockingType::kNone},
      {ControlType::BLOCK, ControlType::BLOCK_THIRD_PARTY,
       DomainBlockingType::k1PES},
      {ControlType::BLOCK, ControlType::BLOCK, DomainBlockingType::kAggressive},
  };

  for (const auto& test_case : kTestCases) {
    SCOPED_TRACE(testing::Message()
                 << static_cast<int>(test_case.ad_control_type)
                 << static_cast<int>(test_case.cosmetic_filtering_control_type)
                 << static_cast<int>(test_case.expected_blocking_type));
    brave_shields::SetAdControlType(map, test_case.ad_control_type, url);
    brave_shields::SetCosmeticFilteringControlType(
        map, test_case.cosmetic_filtering_control_type, url);
    ExpectDomainBlockingType(url, test_case.expected_blocking_type);
  }
}
