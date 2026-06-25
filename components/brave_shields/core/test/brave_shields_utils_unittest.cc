// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"

#include <array>
#include <memory>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "base/test/scoped_feature_list.h"
#include "base/token.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/core/browser/brave_shields_test_utils.h"
#include "brave/components/brave_shields/core/common/brave_shield_utils.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_utils.h"
#include "net/base/features.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

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
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    TestingBrowserProcess::GetGlobal()->SetProfileManager(
        CreateProfileManagerForTest());
    TestingProfile::Builder builder;
    builder.SetPath(temp_dir_.GetPath());
    profile_ = builder.Build();
    g_browser_process->profile_manager()->InitProfileUserPrefs(profile_.get());
  }

  void TearDown() override {
    profile_.reset();
    TestingBrowserProcess::GetGlobal()->SetProfileManager(nullptr);
    content::RunAllTasksUntilIdle();
  }

  TestingProfile* profile() { return profile_.get(); }

  HostContentSettingsMap* map() {
    return HostContentSettingsMapFactory::GetForProfile(profile());
  }

  void ExpectDomainBlockingType(const GURL& url,
                                DomainBlockingType domain_blocking_type) {
    auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
    auto setting = brave_shields::GetDomainBlockingType(map, url);
    EXPECT_EQ(domain_blocking_type, setting);
  }

 private:
  std::unique_ptr<BraveProfileManager> CreateProfileManagerForTest() {
    return std::make_unique<BraveProfileManagerWithoutInit>(
        temp_dir_.GetPath());
  }

  base::ScopedTempDir temp_dir_;
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

  brave_shields::SetBraveShieldsEnabled(map, false, GURL("http://brave.com"));
  // setting should apply to origin
  auto setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                        ContentSettingsType::BRAVE_SHIELDS);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  // setting should apply to different scheme
  setting = map->GetContentSetting(GURL("https://brave.com"), GURL(),
                                   ContentSettingsType::BRAVE_SHIELDS);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  // setting should not apply to default
  setting = map->GetContentSetting(GURL(), GURL(),
                                   ContentSettingsType::BRAVE_SHIELDS);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  GURL host2("http://host2.com");
  GURL host1("http://host1.com");
  // Set shields as disabled for initial value.
  brave_shields::SetBraveShieldsEnabled(map, false, host1);
  // host2.com is enabled by default
  setting =
      map->GetContentSetting(host2, GURL(), ContentSettingsType::BRAVE_SHIELDS);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // Set policy to disable shields for specific domain.
  base::ListValue disabled_list;
  disabled_list.Append("[*.]host2.com");
  disabled_list.Append("*.*");
  profile()->GetTestingPrefService()->SetManagedPref(
      kManagedBraveShieldsDisabledForUrls, std::move(disabled_list));

  base::ListValue enabled_list;
  enabled_list.Append("[*.]host1.com");
  profile()->GetTestingPrefService()->SetManagedPref(
      kManagedBraveShieldsEnabledForUrls, std::move(enabled_list));

  // setting should apply block to origin.
  setting =
      map->GetContentSetting(host2, GURL(), ContentSettingsType::BRAVE_SHIELDS);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  brave_shields::SetBraveShieldsEnabled(map, true, host2);

  // setting should not be changed.
  EXPECT_FALSE(brave_shields::GetBraveShieldsEnabled(map, host2));

  // setting should apply enabled to origin.
  EXPECT_TRUE(brave_shields::GetBraveShieldsEnabled(map, host1));
  brave_shields::SetBraveShieldsEnabled(map, false, host1);
  // setting should not be changed.
  EXPECT_TRUE(brave_shields::GetBraveShieldsEnabled(map, host1));

  // setting should not apply to default
  EXPECT_TRUE(brave_shields::GetBraveShieldsEnabled(map, GURL()));
}

TEST_F(BraveShieldsUtilTest, IsBraveShieldsManaged) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  GURL host2("http://host2.com");
  GURL host1("http://host1.com");
  EXPECT_FALSE(brave_shields::IsBraveShieldsManaged(
      profile()->GetTestingPrefService(), map, host2));

  base::ListValue disabled_list;
  disabled_list.Append("[*.]host2.com");
  profile()->GetTestingPrefService()->SetManagedPref(
      kManagedBraveShieldsDisabledForUrls, std::move(disabled_list));
  // only disabled pref set
  EXPECT_TRUE(brave_shields::IsBraveShieldsManaged(
      profile()->GetTestingPrefService(), map, host2));

  EXPECT_FALSE(brave_shields::IsBraveShieldsManaged(
      profile()->GetTestingPrefService(), map, GURL("http://host1.com")));

  base::ListValue enabled_list;
  enabled_list.Append("[*.]host1.com");
  profile()->GetTestingPrefService()->SetManagedPref(
      kManagedBraveShieldsEnabledForUrls, std::move(enabled_list));

  // both disabled/enabled prefs set
  EXPECT_TRUE(brave_shields::IsBraveShieldsManaged(
      profile()->GetTestingPrefService(), map, host2));

  EXPECT_TRUE(brave_shields::IsBraveShieldsManaged(
      profile()->GetTestingPrefService(), map, host1));

  profile()->GetTestingPrefService()->RemoveManagedPref(
      kManagedBraveShieldsDisabledForUrls);

  // only enabled prefs set
  EXPECT_FALSE(brave_shields::IsBraveShieldsManaged(
      profile()->GetTestingPrefService(), map, host2));

  EXPECT_TRUE(brave_shields::IsBraveShieldsManaged(
      profile()->GetTestingPrefService(), map, host1));
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
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   ContentSettingsType::BRAVE_ADS);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

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
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
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

TEST_F(BraveShieldsUtilTest, GetAdControlType_ManagedPref) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  profile()->GetTestingPrefService()->SetManagedPref(
      prefs::kManagedDefaultBraveAdblockSetting,
      base::Value(CONTENT_SETTING_ALLOW));
  EXPECT_EQ(ControlType::ALLOW, brave_shields::GetAdControlType(map, GURL()));
  EXPECT_EQ(ControlType::ALLOW,
            brave_shields::GetAdControlType(map, GURL("http://brave.com")));

  profile()->GetTestingPrefService()->SetManagedPref(
      prefs::kManagedDefaultBraveAdblockSetting,
      base::Value(CONTENT_SETTING_BLOCK));
  EXPECT_EQ(ControlType::BLOCK, brave_shields::GetAdControlType(map, GURL()));
  EXPECT_EQ(ControlType::BLOCK,
            brave_shields::GetAdControlType(map, GURL("http://brave.com")));
}

/* COOKIE CONTROL */
TEST_F(BraveShieldsUtilTest, SetCookieControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  auto cookies = CookieSettingsFactory::GetForProfile(profile());
  /* ALLOW */
  brave_shields::SetCookieControlType(map, profile()->GetPrefs(),
                                      ControlType::ALLOW, GURL());
  auto setting =
      map->GetContentSetting(GURL(), GURL(), ContentSettingsType::COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  // setting should apply to all urls
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   ContentSettingsType::COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  EXPECT_FALSE(cookies->ShouldBlockThirdPartyCookies());
  setting =
      map->GetContentSetting(GURL("http://brave.com"), GURL("http://brave.com"),
                             ContentSettingsType::COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  /* BLOCK */
  brave_shields::SetCookieControlType(map, profile()->GetPrefs(),
                                      ControlType::BLOCK, GURL());
  setting =
      map->GetContentSetting(GURL(), GURL(), ContentSettingsType::COOKIES);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   ContentSettingsType::COOKIES);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  EXPECT_TRUE(cookies->ShouldBlockThirdPartyCookies());
  // setting should apply to all urls
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   ContentSettingsType::COOKIES);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(GURL("http://brave.com"),
                                   GURL("https://firstParty"),
                                   ContentSettingsType::COOKIES);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  // Ensure BLOCK with kCookieControlsMode == kOff still blocks all cookies.
  profile()->GetPrefs()->SetInteger(
      ::prefs::kCookieControlsMode,
      static_cast<int>(content_settings::CookieControlsMode::kOff));
  EXPECT_FALSE(cookies->ShouldBlockThirdPartyCookies());
  // setting should apply to all urls
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   ContentSettingsType::COOKIES);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);
  setting = map->GetContentSetting(GURL("http://brave.com"),
                                   GURL("https://firstParty"),
                                   ContentSettingsType::COOKIES);
  EXPECT_EQ(CONTENT_SETTING_BLOCK, setting);

  /* BLOCK_THIRD_PARTY */
  brave_shields::SetCookieControlType(map, profile()->GetPrefs(),
                                      ControlType::BLOCK_THIRD_PARTY, GURL());
  setting =
      map->GetContentSetting(GURL(), GURL(), ContentSettingsType::COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  EXPECT_TRUE(cookies->ShouldBlockThirdPartyCookies());
  setting = map->GetContentSetting(GURL(), GURL("https://firstParty"),
                                   ContentSettingsType::COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);

  // setting should apply to all urls
  setting = map->GetContentSetting(GURL("http://brave.com"), GURL(),
                                   ContentSettingsType::COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  EXPECT_TRUE(cookies->ShouldBlockThirdPartyCookies());
  setting =
      map->GetContentSetting(GURL("http://brave.com"), GURL("http://brave.com"),
                             ContentSettingsType::COOKIES);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, setting);
  EXPECT_TRUE(cookies->ShouldBlockThirdPartyCookies());
}

TEST_F(BraveShieldsUtilTest, SetCookieControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  auto cookies = CookieSettingsFactory::GetForProfile(profile());

  brave_shields::SetCookieControlType(
      map, profile()->GetPrefs(), ControlType::ALLOW, GURL("http://brave.com"));
  // override should apply to origin
  auto setting = brave_shields::GetCookieControlType(map, cookies.get(),
                                                     GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);

  // override should also apply to different scheme
  setting = brave_shields::GetCookieControlType(map, cookies.get(),
                                                GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
}

TEST_F(BraveShieldsUtilTest, GetCookieControlType_Default) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  auto cookies = CookieSettingsFactory::GetForProfile(profile());

  EXPECT_EQ(CONTENT_SETTING_ALLOW,
            map->GetContentSetting(GURL::EmptyGURL(), GURL::EmptyGURL(),
                                   ContentSettingsType::BRAVE_COOKIES));
  auto setting =
      brave_shields::GetCookieControlType(map, cookies.get(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
  setting = brave_shields::GetCookieControlType(map, cookies.get(),
                                                GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* ALLOW */
  brave_shields::SetCookieControlType(map, profile()->GetPrefs(),
                                      ControlType::ALLOW, GURL());
  setting = brave_shields::GetCookieControlType(map, cookies.get(), GURL());
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting = brave_shields::GetCookieControlType(map, cookies.get(),
                                                GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);

  /* BLOCK */
  brave_shields::SetCookieControlType(map, profile()->GetPrefs(),
                                      ControlType::BLOCK, GURL());
  setting = brave_shields::GetCookieControlType(map, cookies.get(), GURL());
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting = brave_shields::GetCookieControlType(map, cookies.get(),
                                                GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);

  /* BLOCK_THIRD_PARTY */
  brave_shields::SetCookieControlType(map, profile()->GetPrefs(),
                                      ControlType::BLOCK_THIRD_PARTY, GURL());
  setting = brave_shields::GetCookieControlType(map, cookies.get(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
  setting = brave_shields::GetCookieControlType(map, cookies.get(),
                                                GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  // Setting CONTENT_SETTING_DEFAULT doesn't produce any CHECKS or crashes.
  map->SetDefaultContentSetting(ContentSettingsType::BRAVE_COOKIES,
                                CONTENT_SETTING_DEFAULT);
}

TEST_F(BraveShieldsUtilTest, GetCookieControlType_WithUserSettings) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  auto cookies = CookieSettingsFactory::GetForProfile(profile());

  struct CookieState {
    bool block_first_party = false;
    bool block_third_party = false;
  };

  auto get_effective_cookie_state =
      [](content_settings::CookieSettings* cookie_settings,
         const GURL& url) -> CookieState {
    const auto first_party_blocked =
        cookie_settings->GetCookieSetting(
            url, net::SiteForCookies::FromUrl(url), url,
            net::CookieSettingOverrides(), nullptr) == CONTENT_SETTING_BLOCK;

    const auto third_party_blocked =
        cookie_settings->GetCookieSetting(
            GURL::EmptyGURL(), net::SiteForCookies(), url,
            net::CookieSettingOverrides(), nullptr) == CONTENT_SETTING_BLOCK;

    return {first_party_blocked, third_party_blocked};
  };

  auto cookies_settings =
      get_effective_cookie_state(cookies.get(), GURL("http://brave.com"));
  EXPECT_FALSE(cookies_settings.block_first_party);
  EXPECT_TRUE(cookies_settings.block_third_party);

  // block all
  map->SetContentSettingCustomScope(
      ContentSettingsPattern::Wildcard(),
      ContentSettingsPattern::FromURL(GURL("http://brave.com")),
      ContentSettingsType::COOKIES, CONTENT_SETTING_BLOCK);

  auto setting = brave_shields::GetCookieControlType(map, cookies.get(),
                                                     GURL("http://brave.com"));
  // User settings doesn't affect BRAVE_COOKIES
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  cookies_settings =
      get_effective_cookie_state(cookies.get(), GURL("http://brave.com"));
  EXPECT_TRUE(cookies_settings.block_first_party);
  EXPECT_TRUE(cookies_settings.block_third_party);

  brave_shields::SetCookieControlType(
      map, profile()->GetPrefs(), ControlType::ALLOW, GURL("http://brave.com"));
  setting = brave_shields::GetCookieControlType(map, cookies.get(),
                                                GURL("http://brave.com"));
  // User settings still doesn't affect BRAVE_COOKIES
  EXPECT_EQ(ControlType::ALLOW, setting);

  cookies_settings =
      get_effective_cookie_state(cookies.get(), GURL("http://brave.com"));
  EXPECT_FALSE(cookies_settings.block_first_party);
  EXPECT_FALSE(cookies_settings.block_third_party);
}

TEST_F(BraveShieldsUtilTest, GetCookieControlType_ForOrigin) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  auto cookies = CookieSettingsFactory::GetForProfile(profile());

  auto setting = brave_shields::GetCookieControlType(map, cookies.get(),
                                                     GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* ALLOW */
  brave_shields::SetCookieControlType(
      map, profile()->GetPrefs(), ControlType::ALLOW, GURL("http://brave.com"));
  setting = brave_shields::GetCookieControlType(map, cookies.get(),
                                                GURL("http://brave.com"));
  EXPECT_EQ(ControlType::ALLOW, setting);
  setting = brave_shields::GetCookieControlType(map, cookies.get(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* BLOCK */
  brave_shields::SetCookieControlType(
      map, profile()->GetPrefs(), ControlType::BLOCK, GURL("http://brave.com"));
  setting = brave_shields::GetCookieControlType(map, cookies.get(),
                                                GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK, setting);
  setting = brave_shields::GetCookieControlType(map, cookies.get(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);

  /* BLOCK_THIRD_PARTY */
  brave_shields::SetCookieControlType(map, profile()->GetPrefs(),
                                      ControlType::BLOCK_THIRD_PARTY,
                                      GURL("http://brave.com"));
  setting = brave_shields::GetCookieControlType(map, cookies.get(),
                                                GURL("http://brave.com"));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
  setting = brave_shields::GetCookieControlType(map, cookies.get(), GURL());
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY, setting);
}

TEST_F(BraveShieldsUtilTest,
       GetCookieControlType_ManagedPrefOverridesBlockAllCookies) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  auto cookies = CookieSettingsFactory::GetForProfile(profile());

  // Set initial value to block all cookies.
  brave_shields::SetCookieControlType(map, profile()->GetPrefs(),
                                      ControlType::BLOCK, GURL());
  brave_shields::SetCookieControlType(
      map, profile()->GetPrefs(), ControlType::BLOCK, GURL("http://brave.com"));

  // Set policy to allow cookies.
  profile()->GetTestingPrefService()->SetManagedPref(
      prefs::kManagedDefaultCookiesSetting, base::Value(CONTENT_SETTING_ALLOW));

  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY,
            brave_shields::GetCookieControlType(map, cookies.get(), GURL()));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY,
            brave_shields::GetCookieControlType(map, cookies.get(),
                                                GURL("http://brave.com")));
}

TEST_F(BraveShieldsUtilTest,
       GetCookieControlType_ManagedPrefOverridesBlockThirdPartyCookies) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());
  auto cookies = CookieSettingsFactory::GetForProfile(profile());

  // Set initial value to block third party cookies.
  brave_shields::SetCookieControlType(map, profile()->GetPrefs(),
                                      ControlType::BLOCK_THIRD_PARTY, GURL());
  brave_shields::SetCookieControlType(map, profile()->GetPrefs(),
                                      ControlType::BLOCK_THIRD_PARTY,
                                      GURL("http://brave.com"));

  // Set policy to block all cookies.
  profile()->GetTestingPrefService()->SetManagedPref(
      prefs::kManagedDefaultCookiesSetting, base::Value(CONTENT_SETTING_BLOCK));

  EXPECT_EQ(ControlType::BLOCK,
            brave_shields::GetCookieControlType(map, cookies.get(), GURL()));
  EXPECT_EQ(ControlType::BLOCK,
            brave_shields::GetCookieControlType(map, cookies.get(),
                                                GURL("http://brave.com")));
}

/* FINGERPRINTING CONTROL */
TEST_F(BraveShieldsUtilTest, SetFingerprintingControlType_Default) {
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      brave_shields::features::kBraveShowStrictFingerprintingMode);

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
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      brave_shields::features::kBraveShowStrictFingerprintingMode);

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

TEST_F(BraveShieldsUtilTest, GetFingerprintingControlType_ManagedPref) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  profile()->GetTestingPrefService()->SetManagedPref(
      kManagedDefaultBraveFingerprintingV2, base::Value(CONTENT_SETTING_ALLOW));
  EXPECT_EQ(ControlType::ALLOW,
            brave_shields::GetFingerprintingControlType(map, GURL()));
  EXPECT_EQ(ControlType::ALLOW, brave_shields::GetFingerprintingControlType(
                                    map, GURL("http://brave.com")));

  profile()->GetTestingPrefService()->SetManagedPref(
      kManagedDefaultBraveFingerprintingV2, base::Value(CONTENT_SETTING_ASK));
  EXPECT_EQ(ControlType::DEFAULT,
            brave_shields::GetFingerprintingControlType(map, GURL()));
  EXPECT_EQ(ControlType::DEFAULT, brave_shields::GetFingerprintingControlType(
                                      map, GURL("http://brave.com")));
}

/* HTTPS UPGRADE */
TEST_F(BraveShieldsUtilTest, GetHttpsUpgradeControlType_ManagedPref) {
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile());

  profile()->GetTestingPrefService()->SetManagedPref(
      prefs::kManagedDefaultBraveHttpsUpgrade,
      base::Value(CONTENT_SETTING_ALLOW));
  EXPECT_EQ(ControlType::ALLOW,
            brave_shields::GetHttpsUpgradeControlType(map, GURL()));
  EXPECT_EQ(ControlType::ALLOW, brave_shields::GetHttpsUpgradeControlType(
                                    map, GURL("http://brave.com")));

  profile()->GetTestingPrefService()->SetManagedPref(
      prefs::kManagedDefaultBraveHttpsUpgrade,
      base::Value(CONTENT_SETTING_BLOCK));
  EXPECT_EQ(ControlType::BLOCK,
            brave_shields::GetHttpsUpgradeControlType(map, GURL()));
  EXPECT_EQ(ControlType::BLOCK, brave_shields::GetHttpsUpgradeControlType(
                                    map, GURL("http://brave.com")));

  profile()->GetTestingPrefService()->SetManagedPref(
      prefs::kManagedDefaultBraveHttpsUpgrade,
      base::Value(CONTENT_SETTING_ASK));
  EXPECT_EQ(ControlType::BLOCK_THIRD_PARTY,
            brave_shields::GetHttpsUpgradeControlType(map, GURL()));
  EXPECT_EQ(
      ControlType::BLOCK_THIRD_PARTY,
      brave_shields::GetHttpsUpgradeControlType(map, GURL("http://brave.com")));
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

// Should do 1PES domain blocking if domain blocking feature is enabled.
TEST_F(BraveShieldsUtilTest, GetDomainBlockingType) {
  ExpectDomainBlockingType(GURL("https://brave.com"),
                           DomainBlockingType::k1PES);
}

// Farbling token related tests.

// Unsupported schemes (chrome://, about:blank, data:, invalid URL) must return
// the zero token because shields are not active there.
TEST_F(BraveShieldsUtilTest, FarblingToken_UnsupportedScheme_ReturnsZeroToken) {
  const base::Token zero;
  EXPECT_EQ(zero, brave_shields::GetFarblingToken(
                      map(), GURL("chrome://settings"), {}));
  EXPECT_EQ(zero,
            brave_shields::GetFarblingToken(map(), GURL("about:blank"), {}));
  EXPECT_EQ(zero, brave_shields::GetFarblingToken(
                      map(), GURL("data:text/html,<h1>hello</h1>"), {}));
  EXPECT_EQ(zero, brave_shields::GetFarblingToken(
                      map(), GURL("file:///etc/hosts"), {}));
  EXPECT_EQ(zero, brave_shields::GetFarblingToken(map(), GURL(), {}));
}

// HTTP and HTTPS URLs must produce a non-zero token.
TEST_F(BraveShieldsUtilTest, FarblingToken_HttpAndHttps_ReturnNonZeroToken) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  EXPECT_FALSE(
      brave_shields::GetFarblingToken(map(), GURL("http://example.com"), {})
          .is_zero());
  EXPECT_FALSE(
      brave_shields::GetFarblingToken(map(), GURL("https://example.com"), {})
          .is_zero());
}

// Two URLs with the same origin but different paths must share the same token,
// because the path is stripped when deriving the effective URL.
TEST_F(BraveShieldsUtilTest,
       FarblingToken_SameOrigin_DifferentPaths_SameToken) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  const auto t1 = brave_shields::GetFarblingToken(
      map(), GURL("https://example.com/page1"), {});
  const auto t2 = brave_shields::GetFarblingToken(
      map(), GURL("https://example.com/page2?q=1#anchor"), {});
  EXPECT_EQ(t1, t2);
}

// A blob URL whose inner origin is https://example.com must yield the same
// token as a plain https://example.com URL, because both resolve to the same
// effective origin (https://example.com/).
TEST_F(BraveShieldsUtilTest, FarblingToken_BlobUrl_SameTokenAsOriginUrl) {
  // Use a random seed (0 = random) to exercise the storage path: the first
  // caller writes a random token keyed under https://example.com/; the second
  // caller reads it back regardless of whether it used the blob or plain URL.
  const auto blob_token = brave_shields::GetFarblingToken(
      map(),
      GURL("blob:https://example.com/550e8400-e29b-41d4-a716-446655440000"),
      {});
  const auto https_token = brave_shields::GetFarblingToken(
      map(), GURL("https://example.com/some/path"), {});
  EXPECT_EQ(blob_token, https_token);
}

// A blob URL whose inner origin is a subdomain must resolve to the same token
// as a plain HTTPS URL for that subdomain (the subdomain itself shares a token
// with the root via schemeful-site scoping — see
// FarblingToken_SubdomainAndRoot_ShareToken).
TEST_F(BraveShieldsUtilTest,
       FarblingToken_BlobSubdomainUrl_SameTokenAsSubdomainUrl) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  const auto sub_token = brave_shields::GetFarblingToken(
      map(), GURL("https://sub.example.com"), {});
  const auto blob_sub_token = brave_shields::GetFarblingToken(
      map(), GURL("blob:https://sub.example.com/some-uuid"), {});
  EXPECT_EQ(sub_token, blob_sub_token);
}

// A blob URL for a subdomain must share the token of the root domain.
// BRAVE_SHIELDS_METADATA is registered with
// REQUESTING_SCHEMEFUL_SITE_ONLY_SCOPE, so the content setting is keyed by the
// schemeful site (eTLD+1 + scheme). https://example.com and
// https://sub.example.com resolve to the same schemeful site, so they always
// share one token.
TEST_F(BraveShieldsUtilTest,
       FarblingToken_BlobSubdomainUrl_SameTokenAsRootDomain) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  const auto root_token =
      brave_shields::GetFarblingToken(map(), GURL("https://example.com"), {});
  const auto blob_sub_token = brave_shields::GetFarblingToken(
      map(), GURL("blob:https://sub.example.com/some-uuid"), {});
  EXPECT_EQ(root_token, blob_sub_token);
}

// Two completely different origins must get different tokens.
TEST_F(BraveShieldsUtilTest, FarblingToken_DifferentOrigins_DifferentTokens) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  const auto t1 =
      brave_shields::GetFarblingToken(map(), GURL("https://example.com"), {});
  const auto t2 =
      brave_shields::GetFarblingToken(map(), GURL("https://other.com"), {});
  EXPECT_NE(t1, t2);
}

// A subdomain and its root domain share the same token as they map to same
// schemeful site (eTLD+1 + scheme).
TEST_F(BraveShieldsUtilTest, FarblingToken_SubdomainAndRoot_ShareToken) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  const auto root =
      brave_shields::GetFarblingToken(map(), GURL("https://example.com"), {});
  const auto sub = brave_shields::GetFarblingToken(
      map(), GURL("https://sub.example.com"), {});
  EXPECT_EQ(root, sub);
}

// Calling GetFarblingToken twice for the same URL must return the same token.
// This verifies that the token is persisted in HostContentSettingsMap and not
// regenerated on every call.
TEST_F(BraveShieldsUtilTest, FarblingToken_IsStableAcrossMultipleCalls) {
  const auto t1 =
      brave_shields::GetFarblingToken(map(), GURL("https://example.com"), {});
  const auto t2 =
      brave_shields::GetFarblingToken(map(), GURL("https://example.com"), {});
  EXPECT_EQ(t1, t2);
  EXPECT_FALSE(t1.is_zero());
}

// Providing additional_entropy must produce a token different from the base
// token for the same URL.
TEST_F(BraveShieldsUtilTest, FarblingToken_AdditionalEntropy_ChangesToken) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  const auto base_token =
      brave_shields::GetFarblingToken(map(), GURL("https://example.com"), {});
  constexpr std::array<uint8_t, 4> entropy = {0x01, 0x02, 0x03, 0x04};
  const auto derived_token = brave_shields::GetFarblingToken(
      map(), GURL("https://example.com"), entropy);
  EXPECT_NE(base_token, derived_token);
}

// Two calls with the same URL and the same additional_entropy must produce the
// same derived token (the XOR derivation is deterministic given a stable base).
TEST_F(BraveShieldsUtilTest,
       FarblingToken_SameEntropy_ProducesSameDerivedToken) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  constexpr std::array<uint8_t, 4> entropy = {0x05, 0x06, 0x07, 0x08};
  const auto t1 = brave_shields::GetFarblingToken(
      map(), GURL("https://example.com"), entropy);
  const auto t2 = brave_shields::GetFarblingToken(
      map(), GURL("https://example.com"), entropy);
  EXPECT_EQ(t1, t2);
}

// Two calls with the same URL but different additional_entropy values must
// produce different derived tokens.
TEST_F(BraveShieldsUtilTest,
       FarblingToken_DifferentEntropy_ProducesDifferentDerivedTokens) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  constexpr std::array<uint8_t, 4> entropy_a = {0xAA, 0xBB, 0xCC, 0xDD};
  constexpr std::array<uint8_t, 4> entropy_b = {0x11, 0x22, 0x33, 0x44};
  const auto t1 = brave_shields::GetFarblingToken(
      map(), GURL("https://example.com"), entropy_a);
  const auto t2 = brave_shields::GetFarblingToken(
      map(), GURL("https://example.com"), entropy_b);
  EXPECT_NE(t1, t2);
}

// The base token (no entropy) is unaffected by what another origin's derived
// token looks like: each origin owns its own stored base token.
TEST_F(BraveShieldsUtilTest, FarblingToken_PerOrigin_TokensAreIndependent) {
  brave_shields::ScopedStableFarblingTokensForTesting scoped_seed(1);
  constexpr std::array<uint8_t, 4> entropy = {0x01, 0x02, 0x03, 0x04};
  const auto derived_a =
      brave_shields::GetFarblingToken(map(), GURL("https://a.com"), entropy);
  const auto derived_b =
      brave_shields::GetFarblingToken(map(), GURL("https://b.com"), entropy);
  // Different origins → different base tokens → different derived tokens even
  // with identical entropy.
  EXPECT_NE(derived_a, derived_b);
}
