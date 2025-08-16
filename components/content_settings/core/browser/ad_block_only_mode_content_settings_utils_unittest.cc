/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/ad_block_only_mode_content_settings_utils.h"

#include "base/time/time.h"
#include "components/content_settings/core/browser/content_settings_origin_value_map.h"
#include "components/content_settings/core/common/content_settings_enums.mojom.h"
#include "components/content_settings/core/common/content_settings_metadata.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace content_settings {

class AdBlockOnlyModeContentSettingsUtilsTest : public testing::Test {
 public:
  AdBlockOnlyModeContentSettingsUtilsTest() = default;
  ~AdBlockOnlyModeContentSettingsUtilsTest() override = default;

  void CheckMetadata(const RuleMetaData& metadata) {
    EXPECT_EQ(metadata.session_model(), mojom::SessionModel::DURABLE);
    EXPECT_EQ(metadata.expiration(), base::Time());
    EXPECT_EQ(metadata.lifetime(), base::TimeDelta());
  }
};

TEST_F(AdBlockOnlyModeContentSettingsUtilsTest,
       IsAdBlockOnlyModeContentSettingsType) {
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::JAVASCRIPT, /*is_off_the_record*/ false));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::COOKIES, /*is_off_the_record*/ false));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_COOKIES, /*is_off_the_record*/ false));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_REFERRERS, /*is_off_the_record*/ false));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_ADS, /*is_off_the_record*/ false));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_TRACKERS, /*is_off_the_record*/ false));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_COSMETIC_FILTERING,
      /*is_off_the_record*/ false));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_FINGERPRINTING_V2,
      /*is_off_the_record*/ false));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
      /*is_off_the_record*/ false));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_HTTPS_UPGRADE, /*is_off_the_record*/ false));

  EXPECT_FALSE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::GEOLOCATION, /*is_off_the_record*/ false));
  EXPECT_FALSE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::NOTIFICATIONS, /*is_off_the_record*/ false));
  EXPECT_FALSE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::IMAGES, /*is_off_the_record*/ false));
  EXPECT_FALSE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_SHIELDS, /*is_off_the_record*/ false));
}

TEST_F(AdBlockOnlyModeContentSettingsUtilsTest,
       IsAdBlockOnlyModeContentSettingsTypeOffTheRecord) {
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::JAVASCRIPT, /*is_off_the_record*/ true));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(ContentSettingsType::COOKIES,
                                                   /*is_off_the_record*/ true));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_COOKIES, /*is_off_the_record*/ true));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_REFERRERS, /*is_off_the_record*/ true));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_ADS, /*is_off_the_record*/ true));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_TRACKERS, /*is_off_the_record*/ true));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_COSMETIC_FILTERING,
      /*is_off_the_record*/ true));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_FINGERPRINTING_V2,
      /*is_off_the_record*/ true));
  EXPECT_TRUE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
      /*is_off_the_record*/ true));

  EXPECT_FALSE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_HTTPS_UPGRADE, /*is_off_the_record*/ true));
  EXPECT_FALSE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::GEOLOCATION, /*is_off_the_record*/ true));
  EXPECT_FALSE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::NOTIFICATIONS, /*is_off_the_record*/ true));
  EXPECT_FALSE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::IMAGES, /*is_off_the_record*/ true));
  EXPECT_FALSE(IsAdBlockOnlyModeContentSettingsType(
      ContentSettingsType::BRAVE_SHIELDS, /*is_off_the_record*/ true));
}

TEST_F(AdBlockOnlyModeContentSettingsUtilsTest, FillAdBlockOnlyModeRules) {
  OriginValueMap ad_block_only_mode_rules;
  FillAdBlockOnlyModeRules(ad_block_only_mode_rules);

  {
    base::AutoLock auto_lock(ad_block_only_mode_rules.GetLock());
    auto rule = ad_block_only_mode_rules.GetRule(
        GURL("https://example.com"), GURL("https://example.com"),
        ContentSettingsType::JAVASCRIPT);
    ASSERT_TRUE(rule);
    EXPECT_EQ(ValueToContentSetting(rule->value), CONTENT_SETTING_ALLOW);
    CheckMetadata(rule->metadata);
  }

  {
    base::AutoLock auto_lock(ad_block_only_mode_rules.GetLock());
    auto rule = ad_block_only_mode_rules.GetRule(GURL("https://example.com"),
                                                 GURL("https://example.com"),
                                                 ContentSettingsType::COOKIES);
    ASSERT_TRUE(rule);
    EXPECT_EQ(ValueToContentSetting(rule->value), CONTENT_SETTING_ALLOW);
    CheckMetadata(rule->metadata);
  }

  {
    base::AutoLock auto_lock(ad_block_only_mode_rules.GetLock());
    auto rule = ad_block_only_mode_rules.GetRule(
        GURL("https://example.com"), GURL("https://example.com"),
        ContentSettingsType::BRAVE_COOKIES);
    ASSERT_TRUE(rule);
    EXPECT_EQ(ValueToContentSetting(rule->value), CONTENT_SETTING_ALLOW);
    CheckMetadata(rule->metadata);
  }

  {
    base::AutoLock auto_lock(ad_block_only_mode_rules.GetLock());
    auto rule = ad_block_only_mode_rules.GetRule(
        GURL("https://example.com"), GURL("https://example.com"),
        ContentSettingsType::BRAVE_REFERRERS);
    ASSERT_TRUE(rule);
    EXPECT_EQ(ValueToContentSetting(rule->value), CONTENT_SETTING_ALLOW);
    CheckMetadata(rule->metadata);
  }

  {
    base::AutoLock auto_lock(ad_block_only_mode_rules.GetLock());
    auto rule = ad_block_only_mode_rules.GetRule(
        GURL("https://example.com"), GURL("https://example.com"),
        ContentSettingsType::BRAVE_ADS);
    ASSERT_TRUE(rule);
    EXPECT_EQ(ValueToContentSetting(rule->value), CONTENT_SETTING_BLOCK);
    CheckMetadata(rule->metadata);
  }

  {
    base::AutoLock auto_lock(ad_block_only_mode_rules.GetLock());
    auto rule = ad_block_only_mode_rules.GetRule(
        GURL("https://example.com"), GURL("https://example.com"),
        ContentSettingsType::BRAVE_TRACKERS);
    ASSERT_TRUE(rule);
    EXPECT_EQ(ValueToContentSetting(rule->value), CONTENT_SETTING_ALLOW);
    CheckMetadata(rule->metadata);
  }

  {
    base::AutoLock auto_lock(ad_block_only_mode_rules.GetLock());
    auto rule = ad_block_only_mode_rules.GetRule(
        GURL("https://example.com"), GURL("https://example.com"),
        ContentSettingsType::BRAVE_COSMETIC_FILTERING);
    ASSERT_TRUE(rule);
    EXPECT_EQ(ValueToContentSetting(rule->value), CONTENT_SETTING_ALLOW);
    CheckMetadata(rule->metadata);
  }

  {
    base::AutoLock auto_lock(ad_block_only_mode_rules.GetLock());
    auto rule = ad_block_only_mode_rules.GetRule(
        GURL("https://example.com"), GURL("https://example.com"),
        ContentSettingsType::BRAVE_FINGERPRINTING_V2);
    ASSERT_TRUE(rule);
    EXPECT_EQ(ValueToContentSetting(rule->value), CONTENT_SETTING_ALLOW);
    CheckMetadata(rule->metadata);
  }

  {
    base::AutoLock auto_lock(ad_block_only_mode_rules.GetLock());
    auto rule = ad_block_only_mode_rules.GetRule(
        GURL("https://example.com"), GURL("https://example.com"),
        ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE);
    ASSERT_TRUE(rule);
    EXPECT_EQ(ValueToContentSetting(rule->value), CONTENT_SETTING_ALLOW);
    CheckMetadata(rule->metadata);
  }

  {
    base::AutoLock auto_lock(ad_block_only_mode_rules.GetLock());
    auto rule = ad_block_only_mode_rules.GetRule(
        GURL("https://example.com"), GURL("https://example.com"),
        ContentSettingsType::BRAVE_HTTPS_UPGRADE);
    ASSERT_TRUE(rule);
    EXPECT_EQ(ValueToContentSetting(rule->value), CONTENT_SETTING_ASK);
    CheckMetadata(rule->metadata);
  }
}

}  // namespace content_settings
