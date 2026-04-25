/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/browsing_data/brave_browsing_data_remover_delegate.h"

#include <memory>
#include <utility>

#include "base/memory/scoped_refptr.h"
#include "base/test/test_future.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/content_settings_registry.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/browser/website_settings_registry.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browsing_data_filter_builder.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

#if BUILDFLAG(ENABLE_PSST)
#include "brave/components/psst/common/features.h"
#endif

class BraveBrowsingDataRemoverDelegateTest : public testing::Test {
 public:
  void SetUp() override {
    profile_ = std::make_unique<TestingProfile>();
    map_ = HostContentSettingsMapFactory::GetForProfile(profile());
  }

  Profile* profile() { return profile_.get(); }

  HostContentSettingsMap* map() { return map_.get(); }

  BraveBrowsingDataRemoverDelegate* delegate() {
    return static_cast<BraveBrowsingDataRemoverDelegate*>(
        profile()->GetBrowsingDataRemoverDelegate());
  }

  int GetShieldsSettingsCount() {
    int shields_settings_count = 0;
    for (const auto& content_type :
         content_settings::GetShieldsContentSettingsTypes()) {
      ContentSettingsForOneType settings =
          map()->GetSettingsForOneType(content_type);
      shields_settings_count += settings.size();
    }
    return shields_settings_count;
  }

 private:
  content::BrowserTaskEnvironment task_environment_;

  std::unique_ptr<TestingProfile> profile_;
  scoped_refptr<HostContentSettingsMap> map_;
};

TEST_F(BraveBrowsingDataRemoverDelegateTest, ShieldsSettingsClearTest) {
  const GURL kBraveURL("https://www.brave.com");
  const GURL kBatURL("https://basicattentiontoken.org");
  const GURL kGoogleURL("https://www.google.com");
  const GURL kAbcURL("https://www.abc.com");

  // defaults
  int start_count = GetShieldsSettingsCount();

  // Three settings are added.
  map()->SetContentSettingDefaultScope(
      kBraveURL, GURL(), ContentSettingsType::BRAVE_HTTP_UPGRADABLE_RESOURCES,
      CONTENT_SETTING_ALLOW);
  map()->SetContentSettingDefaultScope(
      kBatURL, GURL(), ContentSettingsType::BRAVE_FINGERPRINTING_V2,
      CONTENT_SETTING_ALLOW);
  map()->SetContentSettingCustomScope(
      brave_shields::GetPatternFromURL(kGoogleURL),
      ContentSettingsPattern::Wildcard(), ContentSettingsType::JAVASCRIPT,
      CONTENT_SETTING_BLOCK);

  const base::Time kNow = base::Time::Now();
  const base::Time k1DaysOld = kNow - base::Days(1);

  // Check current shields settings count is the defaults plus 2 and zero after
  // clearing 1 day time range.
  EXPECT_EQ(2 + start_count, GetShieldsSettingsCount());
  delegate()->ClearShieldsSettings(k1DaysOld, kNow);
  EXPECT_EQ(start_count, GetShieldsSettingsCount());
}

TEST_F(BraveBrowsingDataRemoverDelegateTest, ShieldsSettingsKeepDefaults) {
  brave_shields::SetAdControlType(map(), brave_shields::ControlType::BLOCK,
                                  GURL());
  brave_shields::SetCosmeticFilteringControlType(
      map(), brave_shields::ControlType::BLOCK, GURL());

  EXPECT_EQ(brave_shields::DomainBlockingType::kAggressive,
            brave_shields::GetDomainBlockingType(map(), GURL()));
  auto filter_builder = content::BrowsingDataFilterBuilder::Create(
      content::BrowsingDataFilterBuilder::Mode::kPreserve);
  base::test::TestFuture<uint64_t> complete_future;
  profile()->GetBrowsingDataRemoverDelegate()->RemoveEmbedderData(
      /*delete_begin=*/base::Time::Min(),
      /*delete_end=*/base::Time::Max(),
      /*remove_mask=*/
      chrome_browsing_data_remover::DATA_TYPE_CONTENT_SETTINGS,
      filter_builder.get(),
      /*origin_type_mask=*/1, complete_future.GetCallback());
  EXPECT_EQ(0u, complete_future.Get());

  EXPECT_EQ(brave_shields::DomainBlockingType::kAggressive,
            brave_shields::GetDomainBlockingType(map(), GURL()));
}

TEST_F(BraveBrowsingDataRemoverDelegateTest, ShieldsSettingsCookiesClearing) {
  // Allow all cookies by default.
  brave_shields::SetCookieControlType(
      map(), profile()->GetPrefs(), brave_shields::ControlType::ALLOW, GURL());

  // Block all cookies on example.com
  brave_shields::SetCookieControlType(map(), profile()->GetPrefs(),
                                      brave_shields::BLOCK,
                                      GURL("https://example.com"));

  auto filter_builder = content::BrowsingDataFilterBuilder::Create(
      content::BrowsingDataFilterBuilder::Mode::kPreserve);

  base::test::TestFuture<uint64_t> complete_future;

  profile()->GetBrowsingDataRemoverDelegate()->RemoveEmbedderData(
      /*delete_begin=*/base::Time::Min(),
      /*delete_end=*/base::Time::Max(),
      /*remove_mask=*/
      chrome_browsing_data_remover::DATA_TYPE_CONTENT_SETTINGS,
      filter_builder.get(),
      /*origin_type_mask=*/1, complete_future.GetCallback());
  EXPECT_EQ(0u, complete_future.Get());

  auto cookie_settings = CookieSettingsFactory::GetForProfile(profile());

  // The default is not changed.
  EXPECT_EQ(brave_shields::ControlType::ALLOW,
            brave_shields::GetCookieControlType(map(), cookie_settings.get(),
                                                GURL()));

  // Default after clearing on the example.com.
  EXPECT_EQ(brave_shields::ControlType::ALLOW,
            brave_shields::GetCookieControlType(map(), cookie_settings.get(),
                                                GURL("https://example.com")));

  // Changing the default settings affects example.com
  brave_shields::SetCookieControlType(map(), profile()->GetPrefs(),
                                      brave_shields::BLOCK, GURL());
  EXPECT_EQ(brave_shields::ControlType::BLOCK,
            brave_shields::GetCookieControlType(map(), cookie_settings.get(),
                                                GURL("https://example.com")));
}

TEST_F(BraveBrowsingDataRemoverDelegateTest, FingerpintV2ClearBalancedPattern) {
  // Simulate sync updates pref directly.
  base::DictValue value;
  value.Set("expiration", "0");
  value.Set("last_modified", "13304670271801570");
  value.Set("setting", CONTENT_SETTING_BLOCK);
  base::DictValue update;
  update.Set("*,https://balanced/*", std::move(value));

  profile()->GetPrefs()->SetDict(
      "profile.content_settings.exceptions.fingerprintingV2",
      std::move(update));

  EXPECT_EQ(
      CONTENT_SETTING_BLOCK,
      map()->GetContentSetting(GURL(), GURL("https://balanced/"),
                               ContentSettingsType::BRAVE_FINGERPRINTING_V2));

  auto filter_builder = content::BrowsingDataFilterBuilder::Create(
      content::BrowsingDataFilterBuilder::Mode::kPreserve);

  base::test::TestFuture<uint64_t> complete_future;
  profile()->GetBrowsingDataRemoverDelegate()->RemoveEmbedderData(
      /*delete_begin=*/base::Time::Min(),
      /*delete_end=*/base::Time::Max(),
      /*remove_mask=*/
      chrome_browsing_data_remover::DATA_TYPE_CONTENT_SETTINGS,
      filter_builder.get(),
      /*origin_type_mask=*/1, complete_future.GetCallback());
  EXPECT_EQ(0u, complete_future.Get());

  EXPECT_EQ(
      CONTENT_SETTING_ASK,  // The default value for BRAVE_FINGERPRINTING_V2
      map()->GetContentSetting(GURL(), GURL("https://balanced/"),
                               ContentSettingsType::BRAVE_FINGERPRINTING_V2));
}

class DataTypeContentSettings_Clearing
    : public BraveBrowsingDataRemoverDelegateTest {
 public:
  bool ShouldNotBeRegistered(ContentSettingsType type) {
    switch (type) {
      default:
        return false;

      case ContentSettingsType::BRAVE_PSST:
        return !BUILDFLAG(ENABLE_PSST);
      case ContentSettingsType::BRAVE_ETHEREUM:
      case ContentSettingsType::BRAVE_SOLANA:
      case ContentSettingsType::BRAVE_CARDANO:
        return !BUILDFLAG(ENABLE_BRAVE_WALLET);
      case ContentSettingsType::BRAVE_WEBCOMPAT_ALL:
        // This value is used to determine the last WebCompat content setting
        // and shouldn't be registered.
        return true;
    }
  }

  ContentSetting GetNonDefaultValidContentSettingValue(
      const content_settings::ContentSettingsInfo* info) {
    for (ContentSetting setting = CONTENT_SETTING_ALLOW;
         setting < CONTENT_SETTING_NUM_SETTINGS;
         setting = static_cast<ContentSetting>(setting + 1)) {
      if (info->GetInitialDefaultSetting() != setting &&
          info->IsSettingValid(setting)) {
        return setting;
      }
    }
    NOTREACHED() << "Failed to get value for "
                 << info->website_settings_info()->name();
  }

  bool ShouldNotClear(ContentSettingsType type) {
    switch (type) {
      default:
        return false;

      case ContentSettingsType::BRAVE_SHIELDS_METADATA:
        // Cleared when DATA_TYPE_COOKIES or DATA_TYPE_HISTORY is selected.
        return true;
    }
  }

 private:
#if BUILDFLAG(ENABLE_PSST)
  base::test::ScopedFeatureList psst_feature_{psst::features::kEnablePsst};
#endif
};

// This test verifies that all Brave-introduced content/website settings
// are properly cleared when clearing `Shields & Sites` browsing data via
// settings.
//
// If this test fails:
// 1. Ensure the corresponding feature is enabled (by adding
//      base::test::ScopedFeatureList your_feature_{feature-name})
// 2. Check for related build flags (or other reason) in ShouldNotBeRegistered
// 3. Add the necessary clearing logic to `BraveRemoveSiteSettingsData()` in:
//  `content_settings/core/browser/brave_content_settings_browsing_data_utils.h`
// 4. Alternatively, add an exception in the `ShouldNotClear()` method above
//    with reason why your content setting shouldn't be cleared.
TEST_F(DataTypeContentSettings_Clearing, CheckAll) {
  const GURL url("https://example.com");

  auto* web_settings_registry =
      content_settings::WebsiteSettingsRegistry::GetInstance();
  auto* content_settings_registry =
      content_settings::ContentSettingsRegistry::GetInstance();

  for (auto type = ContentSettingsType::BRAVE_START;
       type < ContentSettingsType::kMaxValue;
       type = static_cast<ContentSettingsType>(std::to_underlying(type) + 1)) {
    const auto* web_setting = web_settings_registry->Get(type);
    if (ShouldNotBeRegistered(type)) {
      // Registration is skipped for some reason.
      ASSERT_FALSE(web_setting);
      continue;
    }
    ASSERT_TRUE(web_setting)
        << "ContentSetting " << type
        << " is declared but not registered. Please sure you enabled the "
           "feature and added the buildflag check.";

    SCOPED_TRACE(web_setting->name());

    const auto* content_setting = content_settings_registry->Get(type);
    if (!content_setting) {
      // Registered as WebsiteSetting only.
      base::Value value(base::DictValue().Set("test", "value"));
      map()->SetWebsiteSettingDefaultScope(url, GURL::EmptyGURL(), type,
                                           value.Clone());
      EXPECT_EQ(value, map()->GetWebsiteSetting(url, GURL::EmptyGURL(), type));
    } else {
      const auto value = GetNonDefaultValidContentSettingValue(content_setting);
      map()->SetContentSettingDefaultScope(url, GURL::EmptyGURL(), type, value);
      EXPECT_EQ(value, map()->GetContentSetting(url, GURL::EmptyGURL(), type));
    }

    auto filter_builder = content::BrowsingDataFilterBuilder::Create(
        content::BrowsingDataFilterBuilder::Mode::kPreserve);
    base::test::TestFuture<uint64_t> complete_future;
    profile()->GetBrowsingDataRemoverDelegate()->RemoveEmbedderData(
        /*delete_begin=*/base::Time::Min(),
        /*delete_end=*/base::Time::Max(),
        /*remove_mask=*/
        chrome_browsing_data_remover::DATA_TYPE_CONTENT_SETTINGS,
        filter_builder.get(),
        /*origin_type_mask=*/1, complete_future.GetCallback());
    EXPECT_EQ(0u, complete_future.Get());

    content_settings::SettingInfo info;
    map()->GetWebsiteSetting(url, GURL::EmptyGURL(), type, &info);
    if (ShouldNotClear(type)) {
      // The content setting is designed to persist even when browsing data is
      // cleared.
      EXPECT_FALSE(info.primary_pattern.MatchesAllHosts());
    } else {
      // Check the setting has been restored to the default.
      EXPECT_TRUE(info.primary_pattern.MatchesAllHosts());
    }
  }
}
