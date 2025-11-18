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
#include "chrome/browser/browsing_data/chrome_browsing_data_remover_constants.h"
#include "chrome/browser/content_settings/cookie_settings_factory.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/cookie_settings.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browsing_data_filter_builder.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

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
  base::Value::Dict value;
  value.Set("expiration", "0");
  value.Set("last_modified", "13304670271801570");
  value.Set("setting", CONTENT_SETTING_BLOCK);
  base::Value::Dict update;
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
