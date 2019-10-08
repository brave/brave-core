/*  Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/macros.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "build/build_config.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

using brave_shields::ControlType;
using brave_shields::ControlTypeFromString;
using brave_shields::ControlTypeToString;
using brave_shields::GetPatternFromURL;

class BraveShieldsUtilTest : public testing::Test {
 public:
  BraveShieldsUtilTest() = default;
  ~BraveShieldsUtilTest() override = default;

  void SetUp() override { profile_ = std::make_unique<TestingProfile>(); }

  TestingProfile* profile() { return profile_.get(); }

 private:
  content::TestBrowserThreadBundle test_browser_thread_bundle_;
  std::unique_ptr<TestingProfile> profile_;

  DISALLOW_COPY_AND_ASSIGN(BraveShieldsUtilTest);
};

TEST_F(BraveShieldsUtilTest, GetPatternFromURL) {
  // wildcard
  auto pattern = GetPatternFromURL(GURL());
  EXPECT_EQ(ContentSettingsPattern::Wildcard(), pattern);

  // no scheme wildcard
  pattern = GetPatternFromURL(GURL("http://brave.com"));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com/path1")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com/path2")));
  EXPECT_FALSE(pattern.Matches(GURL("https://brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("http://subdomain.brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("http://brave2.com")));

  pattern = GetPatternFromURL(GURL("http://brave.com/path1"));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com/path1")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com/path2")));
  EXPECT_FALSE(pattern.Matches(GURL("https://brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("http://subdomain.brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("http://brave2.com")));

  // with scheme wildcard
  pattern = GetPatternFromURL(GURL("http://brave.com"), true);
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com/path1")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com/path2")));
  EXPECT_TRUE(pattern.Matches(GURL("https://brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("http://subdomain.brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("http://brave2.com")));

  // with port
  pattern = GetPatternFromURL(GURL("http://brave.com:8080"));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com:8080")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com:8080/path1")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com:8080/path2")));
  EXPECT_FALSE(pattern.Matches(GURL("https://brave.com:8080")));
  EXPECT_FALSE(pattern.Matches(GURL("http://brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("https://brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("http://subdomain.brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("http://brave2.com")));

  // with implied port
  pattern = GetPatternFromURL(GURL("https://brianbondy.com"));
  EXPECT_EQ(pattern.ToString(), "https://brianbondy.com:443");
  pattern = GetPatternFromURL(GURL("http://brianbondy.com"));
  EXPECT_EQ(pattern.ToString(), "http://brianbondy.com:80");
  // with specified port
  pattern = GetPatternFromURL(GURL("http://brianbondy.com:8080"));
  EXPECT_EQ(pattern.ToString(), "http://brianbondy.com:8080");

  // with port and scheme wildcard
  // scheme wildcard with explicit port is not a valid pattern so this is
  // identical to "with port"
  pattern = GetPatternFromURL(GURL("http://brave.com:8080"), true);
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com:8080")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com:8080/path1")));
  EXPECT_TRUE(pattern.Matches(GURL("http://brave.com:8080/path2")));
  EXPECT_FALSE(pattern.Matches(GURL("https://brave.com:8080")));
  EXPECT_FALSE(pattern.Matches(GURL("http://brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("https://brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("http://subdomain.brave.com")));
  EXPECT_FALSE(pattern.Matches(GURL("http://brave2.com:8080")));
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
TEST_F(BraveShieldsUtilTest, SetBraveShieldsEnabled_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  // settings should be default
  auto setting =
      map->GetContentSetting(GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
                             brave_shields::kBraveShields);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kBraveShields);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);

  /* enabled */
  brave_shields::SetBraveShieldsEnabled(profile(), true, GURL());
  setting =
      map->GetContentSetting(GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
                             brave_shields::kBraveShields);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // override should apply to all origins
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kBraveShields);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  /* disabled */
  brave_shields::SetBraveShieldsEnabled(profile(), false, GURL());
  setting =
      map->GetContentSetting(GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
                             brave_shields::kBraveShields);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  // override should apply to all origins
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kBraveShields);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  /* DEFAULT */
  brave_shields::ResetBraveShieldsEnabled(profile(), GURL());
  setting =
      map->GetContentSetting(GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
                             brave_shields::kBraveShields);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);

  // override should apply to all origins
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kBraveShields);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
}

TEST_F(BraveShieldsUtilTest, SetBraveShieldsEnabled_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  brave_shields::SetBraveShieldsEnabled(profile(), true,
                                        GURL("http://brave.com"));
  // setting should apply to origin
  auto setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                        CONTENT_SETTINGS_TYPE_PLUGINS,
                                        brave_shields::kBraveShields);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should apply to different scheme
  setting = map->GetContentSetting(GURL("https://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kBraveShields);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should not apply to default
  setting =
      map->GetContentSetting(GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
                             brave_shields::kBraveShields);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
}

TEST_F(BraveShieldsUtilTest, SetBraveShieldsEnabled_IsNotHttpHttps) {
  auto setting = brave_shields::GetBraveShieldsEnabled(
      profile(), GURL("chrome://preferences"));
  EXPECT_EQ(false, setting);
  brave_shields::SetBraveShieldsEnabled(profile(), ControlType::ALLOW,
                                        GURL("chrome://preferences"));
  setting = brave_shields::GetBraveShieldsEnabled(profile(),
                                                  GURL("chrome://preferences"));
  EXPECT_EQ(false, setting);

  setting =
      brave_shields::GetBraveShieldsEnabled(profile(), GURL("about:blank"));
  EXPECT_EQ(false, setting);
  brave_shields::SetBraveShieldsEnabled(profile(), ControlType::ALLOW,
                                        GURL("about:blank"));
  setting =
      brave_shields::GetBraveShieldsEnabled(profile(), GURL("about:blank"));
  EXPECT_EQ(false, setting);
}

TEST_F(BraveShieldsUtilTest, GetBraveShieldsEnabled_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetBraveShieldsEnabled(profile(), GURL());
  EXPECT_EQ(true, setting);

  /* BLOCK */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kBraveShields,
      CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetBraveShieldsEnabled(profile(), GURL());
  EXPECT_EQ(false, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kBraveShields,
      CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetBraveShieldsEnabled(profile(), GURL());
  EXPECT_EQ(true, setting);
}

TEST_F(BraveShieldsUtilTest, GetBraveShieldsEnabled_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetBraveShieldsEnabled(profile(), GURL());
  EXPECT_EQ(true, setting);
  setting = brave_shields::GetBraveShieldsEnabled(profile(),
                                                  GURL("http://brave.com"));
  EXPECT_EQ(true, setting);
  setting = brave_shields::GetBraveShieldsEnabled(profile(),
                                                  GURL("https://brave.com"));
  EXPECT_EQ(true, setting);

  /* BLOCK */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kBraveShields, CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetBraveShieldsEnabled(profile(),
                                                  GURL("http://brave.com/*"));
  EXPECT_EQ(false, setting);
  // https in unchanged
  setting = brave_shields::GetBraveShieldsEnabled(profile(),
                                                  GURL("https://brave.com"));
  EXPECT_EQ(true, setting);
  // default is unchanged
  setting = brave_shields::GetBraveShieldsEnabled(profile(), GURL());
  EXPECT_EQ(true, setting);

  /* ALLOW */
  // change default to block
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kBraveShields,
      CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetBraveShieldsEnabled(profile(),
                                                  GURL("http://brave.com"));
  EXPECT_EQ(false, setting);
  setting = brave_shields::GetBraveShieldsEnabled(profile(),
                                                  GURL("https://brave.com"));
  EXPECT_EQ(false, setting);
  setting = brave_shields::GetBraveShieldsEnabled(profile(), GURL());
  EXPECT_EQ(false, setting);

  // set override to allow
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kBraveShields, CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetBraveShieldsEnabled(profile(),
                                                  GURL("http://brave.com"));
  EXPECT_EQ(true, setting);

  // https in unchanged
  setting = brave_shields::GetBraveShieldsEnabled(profile(),
                                                  GURL("https://brave.com"));
  EXPECT_EQ(false, setting);
  // default is unchanged
  setting = brave_shields::GetBraveShieldsEnabled(profile(), GURL());
  EXPECT_EQ(false, setting);
}

TEST_F(BraveShieldsUtilTest, GetBraveShieldsEnabled_IsNotHttpHttps) {
  auto setting = brave_shields::GetBraveShieldsEnabled(
      profile(), GURL("chrome://preferences"));
  EXPECT_EQ(false, setting);

  setting =
      brave_shields::GetBraveShieldsEnabled(profile(), GURL("about:blank"));
  EXPECT_EQ(false, setting);
}

/* AD CONTROL */
TEST_F(BraveShieldsUtilTest, SetAdControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  // settings should be default
  auto setting = map->GetContentSetting(
      GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kAds);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kAds);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);

  /* ALLOW */
  brave_shields::SetAdControlType(profile(), ControlType::ALLOW, GURL());
  setting = map->GetContentSetting(
      GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kAds);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // override should apply to all origins
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kAds);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  /* BLOCK */
  brave_shields::SetAdControlType(profile(), ControlType::BLOCK, GURL());
  setting = map->GetContentSetting(
      GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kAds);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  // override should apply to all origins
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kAds);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
}

TEST_F(BraveShieldsUtilTest, SetAdControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  brave_shields::SetAdControlType(profile(), ControlType::ALLOW,
                                  GURL("http://brave.com"));
  // setting should apply to origin
  auto setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                        CONTENT_SETTINGS_TYPE_PLUGINS,
                                        brave_shields::kAds);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should not apply to different scheme
  setting = map->GetContentSetting(GURL("https://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kAds);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);

  // setting should not apply to default
  setting = map->GetContentSetting(
      GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kAds);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
}

TEST_F(BraveShieldsUtilTest, GetAdControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetAdControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(ContentSettingsPattern::Wildcard(),
                                    ContentSettingsPattern::Wildcard(),
                                    CONTENT_SETTINGS_TYPE_PLUGINS,
                                    brave_shields::kAds, CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetAdControlType(profile(), GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);

  /* BLOCK */
  map->SetContentSettingCustomScope(ContentSettingsPattern::Wildcard(),
                                    ContentSettingsPattern::Wildcard(),
                                    CONTENT_SETTINGS_TYPE_PLUGINS,
                                    brave_shields::kAds, CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetAdControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);
}

TEST_F(BraveShieldsUtilTest, GetAdControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetAdControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting =
      brave_shields::GetAdControlType(profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting =
      brave_shields::GetAdControlType(profile(), GURL("https://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kAds, CONTENT_SETTING_ALLOW);
  setting =
      brave_shields::GetAdControlType(profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);

  // https in unchanged
  setting =
      brave_shields::GetAdControlType(profile(), GURL("https://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  // default is unchanged
  setting = brave_shields::GetAdControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);

  /* BLOCK */
  // change default to allow
  map->SetContentSettingCustomScope(ContentSettingsPattern::Wildcard(),
                                    ContentSettingsPattern::Wildcard(),
                                    CONTENT_SETTINGS_TYPE_PLUGINS,
                                    brave_shields::kAds, CONTENT_SETTING_ALLOW);
  setting =
      brave_shields::GetAdControlType(profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting =
      brave_shields::GetAdControlType(profile(), GURL("https://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting = brave_shields::GetAdControlType(profile(), GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);

  // set override to block
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kAds, CONTENT_SETTING_BLOCK);
  setting =
      brave_shields::GetAdControlType(profile(), GURL("http://brave.com/*"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  // https in unchanged
  setting =
      brave_shields::GetAdControlType(profile(), GURL("https://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  // default is unchanged
  setting = brave_shields::GetAdControlType(profile(), GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);
}

/* COOKIE CONTROL */
TEST_F(BraveShieldsUtilTest, SetCookieControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  // setting should be default to start with
  auto setting = map->GetContentSetting(
      GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kCookies);
  setting = map->GetContentSetting(
      GURL("http://brave.com"), GURL("https://firstParty"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);

  /* ALLOW */
  brave_shields::SetCookieControlType(profile(), ControlType::ALLOW, GURL());
  setting = map->GetContentSetting(
      GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  // setting should apply to all urls
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  setting = map->GetContentSetting(
      GURL("http://brave.com"), GURL("https://firstParty"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  /* BLOCK */
  brave_shields::SetCookieControlType(profile(), ControlType::BLOCK, GURL());
  setting = map->GetContentSetting(
      GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  // setting should apply to all urls
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(
      GURL("http://brave.com"), GURL("https://firstParty"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  /* BLOCK_THIRD_PARTY */
  brave_shields::SetCookieControlType(profile(), ControlType::BLOCK_THIRD_PARTY,
                                      GURL());
  setting = map->GetContentSetting(
      GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should apply to all urls
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(
      GURL("http://brave.com"), GURL("https://firstParty"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
}

TEST_F(BraveShieldsUtilTest, SetCookieControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  brave_shields::SetCookieControlType(profile(), ControlType::ALLOW,
                                      GURL("http://brave.com"));
  // override should apply to origin
  auto setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                        CONTENT_SETTINGS_TYPE_PLUGINS,
                                        brave_shields::kCookies);
  setting = map->GetContentSetting(
      GURL("http://brave.com"), GURL("https://firstParty"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // override should not apply to different scheme
  setting = map->GetContentSetting(GURL("https://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  setting = map->GetContentSetting(
      GURL("https://brave.com"), GURL("https://firstParty"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);

  // override should not apply to default
  setting = map->GetContentSetting(
      GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  // override should not apply to default
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kCookies);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
}

TEST_F(BraveShieldsUtilTest, GetCookieControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetCookieControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
  setting =
      brave_shields::GetCookieControlType(profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies,
      CONTENT_SETTING_ALLOW);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies,
      CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetCookieControlType(profile(), GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting =
      brave_shields::GetCookieControlType(profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);

  /* BLOCK */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies,
      CONTENT_SETTING_BLOCK);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies,
      CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetCookieControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting =
      brave_shields::GetCookieControlType(profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);

  /* BLOCK_THIRD_PARTY */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies,
      CONTENT_SETTING_BLOCK);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies,
      CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetCookieControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
  setting =
      brave_shields::GetCookieControlType(profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
}

TEST_F(BraveShieldsUtilTest, GetCookieControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting =
      brave_shields::GetCookieControlType(profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kCookies, CONTENT_SETTING_ALLOW);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies,
      CONTENT_SETTING_ALLOW);
  setting =
      brave_shields::GetCookieControlType(profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting = brave_shields::GetCookieControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* BLOCK */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kCookies, CONTENT_SETTING_BLOCK);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies,
      CONTENT_SETTING_BLOCK);
  setting =
      brave_shields::GetCookieControlType(profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting = brave_shields::GetCookieControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* BLOCK_THIRD_PARTY */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kCookies, CONTENT_SETTING_BLOCK);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies,
      CONTENT_SETTING_ALLOW);
  setting =
      brave_shields::GetCookieControlType(profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
  setting = brave_shields::GetCookieControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
}

/* FINGERPRINTING CONTROL */
TEST_F(BraveShieldsUtilTest, SetFingerprintingControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  // setting should be default to start with
  auto setting =
      map->GetContentSetting(GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
                             brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kFingerprinting);
  setting = map->GetContentSetting(
      GURL("http://brave.com"), GURL("https://firstParty"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);

  /* ALLOW */
  brave_shields::SetFingerprintingControlType(profile(), ControlType::ALLOW,
                                              GURL());
  setting =
      map->GetContentSetting(GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
                             brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  // setting should apply to all urls
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  setting = map->GetContentSetting(
      GURL("http://brave.com"), GURL("https://firstParty"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  /* BLOCK */
  brave_shields::SetFingerprintingControlType(profile(), ControlType::BLOCK,
                                              GURL());
  setting =
      map->GetContentSetting(GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
                             brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  // setting should apply to all urls
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(
      GURL("http://brave.com"), GURL("https://firstParty"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  /* BLOCK_THIRD_PARTY */
  brave_shields::SetFingerprintingControlType(
      profile(), ControlType::BLOCK_THIRD_PARTY, GURL());
  setting =
      map->GetContentSetting(GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
                             brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should apply to all urls
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(
      GURL("http://brave.com"), GURL("https://firstParty"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
}

TEST_F(BraveShieldsUtilTest, SetFingerprintingControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  brave_shields::SetFingerprintingControlType(profile(), ControlType::ALLOW,
                                              GURL("http://brave.com"));
  // override should apply to origin
  auto setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                        CONTENT_SETTINGS_TYPE_PLUGINS,
                                        brave_shields::kFingerprinting);
  setting = map->GetContentSetting(
      GURL("http://brave.com"), GURL("https://firstParty"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // override should not apply to different scheme
  setting = map->GetContentSetting(GURL("https://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  setting = map->GetContentSetting(
      GURL("https://brave.com"), GURL("https://firstParty"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);

  // override should not apply to default
  setting =
      map->GetContentSetting(GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
                             brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  // override should not apply to default
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kFingerprinting);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
}

TEST_F(BraveShieldsUtilTest, GetFingerprintingControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetFingerprintingControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
  setting = brave_shields::GetFingerprintingControlType(
      profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      CONTENT_SETTING_ALLOW);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetFingerprintingControlType(profile(), GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting = brave_shields::GetFingerprintingControlType(
      profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);

  /* BLOCK */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      CONTENT_SETTING_BLOCK);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetFingerprintingControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting = brave_shields::GetFingerprintingControlType(
      profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);

  /* BLOCK_THIRD_PARTY */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      CONTENT_SETTING_BLOCK);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetFingerprintingControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
  setting = brave_shields::GetFingerprintingControlType(
      profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
}

TEST_F(BraveShieldsUtilTest, GetFingerprintingControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetFingerprintingControlType(
      profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kFingerprinting, CONTENT_SETTING_ALLOW);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetFingerprintingControlType(
      profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting = brave_shields::GetFingerprintingControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* BLOCK */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kFingerprinting, CONTENT_SETTING_BLOCK);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetFingerprintingControlType(
      profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting = brave_shields::GetFingerprintingControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* BLOCK_THIRD_PARTY */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kFingerprinting, CONTENT_SETTING_BLOCK);
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::FromString("https://firstParty/*"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kFingerprinting,
      CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetFingerprintingControlType(
      profile(), GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
  setting = brave_shields::GetFingerprintingControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
}

/* HTTPSEVERYWHERE CONTROL */
TEST_F(BraveShieldsUtilTest, SetHTTPSEverywhereEnabled_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  // settings should be default
  auto setting =
      map->GetContentSetting(GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
                             brave_shields::kHTTPUpgradableResources);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kHTTPUpgradableResources);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);

  /* disabled */
  brave_shields::SetHTTPSEverywhereEnabled(profile(), false, GURL());
  setting =
      map->GetContentSetting(GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
                             brave_shields::kHTTPUpgradableResources);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // override should apply to all origins
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kHTTPUpgradableResources);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  /* enabled */
  brave_shields::SetHTTPSEverywhereEnabled(profile(), true, GURL());
  setting =
      map->GetContentSetting(GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
                             brave_shields::kHTTPUpgradableResources);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  // override should apply to all origins
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kHTTPUpgradableResources);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
}

TEST_F(BraveShieldsUtilTest, SetHTTPSEverywhereEnabled_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  brave_shields::SetHTTPSEverywhereEnabled(profile(), false,
                                           GURL("http://brave.com"));
  // setting should apply to origin
  auto setting = map->GetContentSetting(
      GURL("http://brave.com"), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kHTTPUpgradableResources);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should apply to different scheme
  setting = map->GetContentSetting(GURL("https://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_PLUGINS,
                                   brave_shields::kHTTPUpgradableResources);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should not apply to default
  setting =
      map->GetContentSetting(GURL(), GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
                             brave_shields::kHTTPUpgradableResources);
  EXPECT_EQ(CONTENT_SETTING_DEFAULT, setting);
}

TEST_F(BraveShieldsUtilTest, GetHTTPSEverywhereEnabled_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetHTTPSEverywhereEnabled(profile(), GURL());
  EXPECT_EQ(true, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kHTTPUpgradableResources,
      CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetHTTPSEverywhereEnabled(profile(), GURL());
  EXPECT_EQ(false, setting);

  /* BLOCK */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kHTTPUpgradableResources,
      CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetHTTPSEverywhereEnabled(profile(), GURL());
  EXPECT_EQ(true, setting);
}

TEST_F(BraveShieldsUtilTest, GetHTTPSEverywhereEnabled_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetHTTPSEverywhereEnabled(profile(), GURL());
  EXPECT_EQ(true, setting);
  setting = brave_shields::GetHTTPSEverywhereEnabled(profile(),
                                                     GURL("http://brave.com"));
  EXPECT_EQ(true, setting);
  setting = brave_shields::GetHTTPSEverywhereEnabled(profile(),
                                                     GURL("https://brave.com"));
  EXPECT_EQ(true, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kHTTPUpgradableResources, CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetHTTPSEverywhereEnabled(profile(),
                                                     GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);

  // https in unchanged
  setting = brave_shields::GetHTTPSEverywhereEnabled(profile(),
                                                     GURL("https://brave.com"));
  EXPECT_EQ(true, setting);
  // default is unchanged
  setting = brave_shields::GetHTTPSEverywhereEnabled(profile(), GURL());
  EXPECT_EQ(true, setting);

  /* BLOCK */
  // change default to allow
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kHTTPUpgradableResources,
      CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetHTTPSEverywhereEnabled(profile(),
                                                     GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting = brave_shields::GetHTTPSEverywhereEnabled(profile(),
                                                     GURL("https://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting = brave_shields::GetHTTPSEverywhereEnabled(profile(), GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);

  // set override to block
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kHTTPUpgradableResources, CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetHTTPSEverywhereEnabled(
      profile(), GURL("http://brave.com/*"));
  EXPECT_EQ(true, setting);
  // https in unchanged
  setting = brave_shields::GetHTTPSEverywhereEnabled(profile(),
                                                     GURL("https://brave.com"));
  EXPECT_EQ(false, setting);
  // default is unchanged
  setting = brave_shields::GetHTTPSEverywhereEnabled(profile(), GURL());
  EXPECT_EQ(false, setting);
}

/* NOSCRIPT CONTROL */
TEST_F(BraveShieldsUtilTest, SetNoScriptControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  // settings should be default
  auto setting = map->GetContentSetting(GURL(), GURL(),
                                        CONTENT_SETTINGS_TYPE_JAVASCRIPT, "");
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_JAVASCRIPT, "");
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  /* BLOCK */
  brave_shields::SetNoScriptControlType(profile(), ControlType::BLOCK, GURL());
  setting = map->GetContentSetting(GURL(), GURL(),
                                   CONTENT_SETTINGS_TYPE_JAVASCRIPT, "");
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  // override should apply to all origins
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_JAVASCRIPT, "");
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  /* ALLOW */
  brave_shields::SetNoScriptControlType(profile(), ControlType::ALLOW, GURL());
  setting = map->GetContentSetting(GURL(), GURL(),
                                   CONTENT_SETTINGS_TYPE_JAVASCRIPT, "");
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // override should apply to all origins
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_JAVASCRIPT, "");
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
}

TEST_F(BraveShieldsUtilTest, SetNoScriptControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  brave_shields::SetNoScriptControlType(profile(), ControlType::BLOCK,
                                        GURL("http://brave.com"));
  // setting should apply to origin
  auto setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                        CONTENT_SETTINGS_TYPE_JAVASCRIPT, "");
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  // setting should not apply to different scheme
  setting = map->GetContentSetting(GURL("https://brave.com"), GURL(),
                                   CONTENT_SETTINGS_TYPE_JAVASCRIPT, "");
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should not apply to default
  setting = map->GetContentSetting(GURL(), GURL(),
                                   CONTENT_SETTINGS_TYPE_JAVASCRIPT, "");
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
}

TEST_F(BraveShieldsUtilTest, GetNoScriptControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetNoScriptControlType(profile(), GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);

  /* BLOCK */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_JAVASCRIPT, "", CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetNoScriptControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);

  /* ALLOW */
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_JAVASCRIPT, "", CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetNoScriptControlType(profile(), GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);
}

TEST_F(BraveShieldsUtilTest, GetNoScriptControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  auto setting = brave_shields::GetNoScriptControlType(profile(), GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting = brave_shields::GetNoScriptControlType(profile(),
                                                  GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting = brave_shields::GetNoScriptControlType(profile(),
                                                  GURL("https://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);

  /* BLOCK */
  // set override to block
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_JAVASCRIPT, "",
      CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetNoScriptControlType(profile(),
                                                  GURL("http://brave.com/*"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  // https in unchanged
  setting = brave_shields::GetNoScriptControlType(profile(),
                                                  GURL("https://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  // default is unchanged
  setting = brave_shields::GetNoScriptControlType(profile(), GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);

  /* ALLOW */
  // change default to block
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(), ContentSettingsPattern::Wildcard(),
      CONTENT_SETTINGS_TYPE_JAVASCRIPT, "", CONTENT_SETTING_BLOCK);
  setting = brave_shields::GetNoScriptControlType(profile(),
                                                  GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting = brave_shields::GetNoScriptControlType(profile(),
                                                  GURL("https://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting = brave_shields::GetNoScriptControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);

  map->SetContentSettingCustomScope(
      ContentSettingsPattern::FromString("http://brave.com/*"),
      ContentSettingsPattern::Wildcard(), CONTENT_SETTINGS_TYPE_JAVASCRIPT, "",
      CONTENT_SETTING_ALLOW);
  setting = brave_shields::GetNoScriptControlType(profile(),
                                                  GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);

  // https in unchanged
  setting = brave_shields::GetNoScriptControlType(profile(),
                                                  GURL("https://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  // default is unchanged
  setting = brave_shields::GetNoScriptControlType(profile(), GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);
}
