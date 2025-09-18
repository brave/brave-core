/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <string>

#include "brave/components/constants/pref_names.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class BraveHostContentSettingsMapTest : public testing::Test {
 public:
  BraveHostContentSettingsMapTest() = default;
  ~BraveHostContentSettingsMapTest() override = default;

 protected:
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(BraveHostContentSettingsMapTest, ManagedDefaultContentSetting) {
  TestingProfile profile;
  HostContentSettingsMap* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(&profile);
  sync_preferences::TestingPrefServiceSyncable* prefs =
      profile.GetTestingPrefService();

  EXPECT_EQ(CONTENT_SETTING_ASK,
            host_content_settings_map->GetDefaultContentSetting(
                ContentSettingsType::BRAVE_FINGERPRINTING_V2));

  prefs->SetManagedPref(kManagedDefaultBraveFingerprintingV2,
                        std::make_unique<base::Value>(CONTENT_SETTING_BLOCK));
  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            host_content_settings_map->GetDefaultContentSetting(
                ContentSettingsType::BRAVE_FINGERPRINTING_V2));

  prefs->RemoveManagedPref(kManagedDefaultBraveFingerprintingV2);
  EXPECT_EQ(CONTENT_SETTING_ASK,
            host_content_settings_map->GetDefaultContentSetting(
                ContentSettingsType::BRAVE_FINGERPRINTING_V2));
}

TEST_F(BraveHostContentSettingsMapTest,
       GetNonDefaultContentSettingsIfTypeManaged) {
  TestingProfile profile;
  HostContentSettingsMap* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(&profile);
  sync_preferences::TestingPrefServiceSyncable* prefs =
      profile.GetTestingPrefService();

  GURL host("http://example.com/");
  host_content_settings_map->SetContentSettingDefaultScope(
      host, GURL(), ContentSettingsType::BRAVE_FINGERPRINTING_V2,
      CONTENT_SETTING_BLOCK);

  EXPECT_EQ(CONTENT_SETTING_ASK,
            host_content_settings_map->GetDefaultContentSetting(
                ContentSettingsType::BRAVE_FINGERPRINTING_V2));

  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            host_content_settings_map->GetContentSetting(
                host, host, ContentSettingsType::BRAVE_FINGERPRINTING_V2));

  prefs->SetManagedPref(kManagedDefaultBraveFingerprintingV2,
                        std::make_unique<base::Value>(CONTENT_SETTING_ALLOW));
  EXPECT_EQ(CONTENT_SETTING_ALLOW,
            host_content_settings_map->GetContentSetting(
                host, host, ContentSettingsType::BRAVE_FINGERPRINTING_V2));
}

TEST_F(BraveHostContentSettingsMapTest,
       ManagedDefaultContentSettingIgnoreUserPattern) {
  TestingProfile profile;
  HostContentSettingsMap* host_content_settings_map =
      HostContentSettingsMapFactory::GetForProfile(&profile);
  sync_preferences::TestingPrefServiceSyncable* prefs =
      profile.GetTestingPrefService();

  GURL host("http://example.com/");
  host_content_settings_map->SetContentSettingDefaultScope(
      host, GURL(), ContentSettingsType::BRAVE_FINGERPRINTING_V2,
      CONTENT_SETTING_ALLOW);

  EXPECT_EQ(CONTENT_SETTING_ASK,
            host_content_settings_map->GetDefaultContentSetting(
                ContentSettingsType::BRAVE_FINGERPRINTING_V2));

  EXPECT_EQ(CONTENT_SETTING_ALLOW,
            host_content_settings_map->GetContentSetting(
                host, host, ContentSettingsType::BRAVE_FINGERPRINTING_V2));

  prefs->SetManagedPref(kManagedDefaultBraveFingerprintingV2,
                        std::make_unique<base::Value>(CONTENT_SETTING_BLOCK));
  EXPECT_EQ(CONTENT_SETTING_BLOCK,
            host_content_settings_map->GetContentSetting(
                host, host, ContentSettingsType::BRAVE_FINGERPRINTING_V2));

  prefs->RemoveManagedPref(kManagedDefaultBraveFingerprintingV2);
  EXPECT_EQ(CONTENT_SETTING_ALLOW,
            host_content_settings_map->GetContentSetting(
                host, host, ContentSettingsType::BRAVE_FINGERPRINTING_V2));
}
