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

  void VerifyAdBlockOnlyModeTypes(bool is_off_the_record) {
    EXPECT_TRUE(IsAdBlockOnlyModeType(ContentSettingsType::JAVASCRIPT,
                                      is_off_the_record));
    EXPECT_TRUE(
        IsAdBlockOnlyModeType(ContentSettingsType::COOKIES, is_off_the_record));
    EXPECT_TRUE(IsAdBlockOnlyModeType(ContentSettingsType::BRAVE_COOKIES,
                                      is_off_the_record));
    EXPECT_TRUE(IsAdBlockOnlyModeType(ContentSettingsType::BRAVE_REFERRERS,
                                      is_off_the_record));
    EXPECT_TRUE(IsAdBlockOnlyModeType(ContentSettingsType::BRAVE_ADS,
                                      is_off_the_record));
    EXPECT_TRUE(IsAdBlockOnlyModeType(ContentSettingsType::BRAVE_TRACKERS,
                                      is_off_the_record));
    EXPECT_TRUE(IsAdBlockOnlyModeType(
        ContentSettingsType::BRAVE_COSMETIC_FILTERING, is_off_the_record));
    EXPECT_TRUE(IsAdBlockOnlyModeType(
        ContentSettingsType::BRAVE_FINGERPRINTING_V2, is_off_the_record));
    EXPECT_TRUE(IsAdBlockOnlyModeType(
        ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE, is_off_the_record));

    if (!is_off_the_record) {
      EXPECT_TRUE(IsAdBlockOnlyModeType(
          ContentSettingsType::BRAVE_HTTPS_UPGRADE, is_off_the_record));
    } else {
      EXPECT_FALSE(IsAdBlockOnlyModeType(
          ContentSettingsType::BRAVE_HTTPS_UPGRADE, is_off_the_record));
    }
  }

  void VerifyNonAdBlockOnlyModeTypes(bool is_off_the_record) {
    EXPECT_FALSE(IsAdBlockOnlyModeType(ContentSettingsType::GEOLOCATION,
                                       is_off_the_record));
    EXPECT_FALSE(IsAdBlockOnlyModeType(ContentSettingsType::NOTIFICATIONS,
                                       is_off_the_record));
    EXPECT_FALSE(
        IsAdBlockOnlyModeType(ContentSettingsType::IMAGES, is_off_the_record));
    EXPECT_FALSE(IsAdBlockOnlyModeType(ContentSettingsType::BRAVE_SHIELDS,
                                       is_off_the_record));
  }

  void VerifyMetaDataExpectation(const RuleMetaData& metadata) {
    EXPECT_EQ(metadata.session_model(), mojom::SessionModel::DURABLE);
    EXPECT_EQ(metadata.expiration(), base::Time());
    EXPECT_EQ(metadata.lifetime(), base::TimeDelta());
  }

  void VerifyAdBlockOnlyModeRule(OriginValueMap& ad_block_only_mode_rules,
                                 ContentSettingsType type,
                                 ContentSetting value) {
    base::AutoLock auto_lock(ad_block_only_mode_rules.GetLock());
    auto rule = ad_block_only_mode_rules.GetRule(
        GURL("https://example.com"), GURL("https://example.com"), type);
    EXPECT_TRUE(rule);
    EXPECT_EQ(ValueToContentSetting(rule->value), value);
    VerifyMetaDataExpectation(rule->metadata);
  }

  void VerifyNonAdBlockOnlyModeRule(OriginValueMap& ad_block_only_mode_rules,
                                    ContentSettingsType type) {
    base::AutoLock auto_lock(ad_block_only_mode_rules.GetLock());
    auto rule = ad_block_only_mode_rules.GetRule(
        GURL("https://example.com"), GURL("https://example.com"), type);
    EXPECT_FALSE(rule);
  }
};

TEST_F(AdBlockOnlyModeContentSettingsUtilsTest, IsAdBlockOnlyModeType) {
  VerifyAdBlockOnlyModeTypes(/*is_off_the_record*/ false);
  VerifyNonAdBlockOnlyModeTypes(/*is_off_the_record*/ false);
}

TEST_F(AdBlockOnlyModeContentSettingsUtilsTest,
       IsAdBlockOnlyModeTypeWhenOffTheRecord) {
  VerifyAdBlockOnlyModeTypes(/*is_off_the_record*/ true);
  VerifyNonAdBlockOnlyModeTypes(/*is_off_the_record*/ true);
}

TEST_F(AdBlockOnlyModeContentSettingsUtilsTest, VerifyAdBlockOnlyModeRules) {
  OriginValueMap ad_block_only_mode_rules;
  SetAdBlockOnlyModeRules(ad_block_only_mode_rules);

  VerifyAdBlockOnlyModeRule(ad_block_only_mode_rules,
                            ContentSettingsType::JAVASCRIPT,
                            CONTENT_SETTING_ALLOW);

  VerifyAdBlockOnlyModeRule(ad_block_only_mode_rules,
                            ContentSettingsType::COOKIES,
                            CONTENT_SETTING_ALLOW);

  VerifyAdBlockOnlyModeRule(ad_block_only_mode_rules,
                            ContentSettingsType::BRAVE_COOKIES,
                            CONTENT_SETTING_ALLOW);

  VerifyAdBlockOnlyModeRule(ad_block_only_mode_rules,
                            ContentSettingsType::BRAVE_REFERRERS,
                            CONTENT_SETTING_ALLOW);

  VerifyAdBlockOnlyModeRule(ad_block_only_mode_rules,
                            ContentSettingsType::BRAVE_ADS,
                            CONTENT_SETTING_BLOCK);

  VerifyAdBlockOnlyModeRule(ad_block_only_mode_rules,
                            ContentSettingsType::BRAVE_TRACKERS,
                            CONTENT_SETTING_ALLOW);

  VerifyAdBlockOnlyModeRule(ad_block_only_mode_rules,
                            ContentSettingsType::BRAVE_COSMETIC_FILTERING,
                            CONTENT_SETTING_ALLOW);

  VerifyAdBlockOnlyModeRule(ad_block_only_mode_rules,
                            ContentSettingsType::BRAVE_FINGERPRINTING_V2,
                            CONTENT_SETTING_ALLOW);

  VerifyAdBlockOnlyModeRule(ad_block_only_mode_rules,
                            ContentSettingsType::BRAVE_REMEMBER_1P_STORAGE,
                            CONTENT_SETTING_ALLOW);

  VerifyAdBlockOnlyModeRule(ad_block_only_mode_rules,
                            ContentSettingsType::BRAVE_HTTPS_UPGRADE,
                            CONTENT_SETTING_ASK);
}

TEST_F(AdBlockOnlyModeContentSettingsUtilsTest, VerifyNonAdBlockOnlyModeRules) {
  OriginValueMap ad_block_only_mode_rules;
  SetAdBlockOnlyModeRules(ad_block_only_mode_rules);

  VerifyNonAdBlockOnlyModeRule(ad_block_only_mode_rules,
                               ContentSettingsType::GEOLOCATION);

  VerifyNonAdBlockOnlyModeRule(ad_block_only_mode_rules,
                               ContentSettingsType::NOTIFICATIONS);

  VerifyNonAdBlockOnlyModeRule(ad_block_only_mode_rules,
                               ContentSettingsType::IMAGES);

  VerifyNonAdBlockOnlyModeRule(ad_block_only_mode_rules,
                               ContentSettingsType::BRAVE_SHIELDS);
}

}  // namespace content_settings
