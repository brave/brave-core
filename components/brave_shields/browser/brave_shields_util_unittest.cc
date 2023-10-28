/*  Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "base/files/scoped_temp_dir.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/components/brave_shields/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/constants/pref_names.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
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
  BraveShieldsUtilTest() : local_state_(TestingBrowserProcess::GetGlobal()) {}
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
  ScopedTestingLocalState local_state_;
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
  base::Value::List disabled_list;
  disabled_list.Append("[*.]host2.com");
  disabled_list.Append("*.*");
  profile()->GetTestingPrefService()->SetManagedPref(
      kManagedBraveShieldsDisabledForUrls, std::move(disabled_list));

  base::Value::List enabled_list;
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

  base::Value::List disabled_list;
  disabled_list.Append("[*.]host2.com");
  profile()->GetTestingPrefService()->SetManagedPref(
      kManagedBraveShieldsDisabledForUrls, std::move(disabled_list));
  // only disabled pref set
  EXPECT_TRUE(brave_shields::IsBraveShieldsManaged(
      profile()->GetTestingPrefService(), map, host2));

  EXPECT_FALSE(brave_shields::IsBraveShieldsManaged(
      profile()->GetTestingPrefService(), map, GURL("http://host1.com")));

  base::Value::List enabled_list;
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
        cookie_settings->GetCookieSetting(url, url,
                                          net::CookieSettingOverrides(),
                                          nullptr) == CONTENT_SETTING_BLOCK;

    const auto third_party_blocked =
        cookie_settings->GetCookieSetting(GURL::EmptyGURL(), url,
                                          net::CookieSettingOverrides(),
                                          nullptr) == CONTENT_SETTING_BLOCK;

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
