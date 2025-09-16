/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/components/constants/pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/content_settings_policy_provider.h"
#include "components/content_settings/core/browser/content_settings_rule.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/content_settings/core/common/content_settings_utils.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content_settings {

class BraveContentSettingsPolicyProviderTest : public testing::Test {
 public:
  BraveContentSettingsPolicyProviderTest() = default;
  ~BraveContentSettingsPolicyProviderTest() override = default;

 private:
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(BraveContentSettingsPolicyProviderTest,
       ManagedDefaultBraveFingerprintingV2) {
  TestingProfile profile;
  sync_preferences::TestingPrefServiceSyncable* prefs =
      profile.GetTestingPrefService();
  PolicyProvider provider(prefs);

  prefs->SetManagedPref(kManagedDefaultBraveFingerprintingV2,
                        std::make_unique<base::Value>(CONTENT_SETTING_ALLOW));

  std::unique_ptr<RuleIterator> rule_iterator(provider.GetRuleIterator(
      ContentSettingsType::BRAVE_FINGERPRINTING_V2, false,
      content_settings::PartitionKey::GetDefaultForTesting()));
  EXPECT_TRUE(rule_iterator->HasNext());
  std::unique_ptr<Rule> rule = rule_iterator->Next();
  EXPECT_FALSE(rule_iterator->HasNext());

  EXPECT_EQ(ContentSettingsPattern::Wildcard(), rule->primary_pattern);
  EXPECT_EQ(ContentSettingsPattern::Wildcard(), rule->secondary_pattern);
  EXPECT_EQ(CONTENT_SETTING_ALLOW, ValueToContentSetting(rule->value));

  provider.ShutdownOnUIThread();
}

}  // namespace content_settings
