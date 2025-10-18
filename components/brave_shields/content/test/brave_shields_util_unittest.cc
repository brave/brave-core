// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/content/browser/brave_shields_util.h"

#include "brave/components/constants/pref_names.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

class BraveShieldsUtilTest : public testing::Test {
 public:
  BraveShieldsUtilTest() = default;
  ~BraveShieldsUtilTest() override = default;

  HostContentSettingsMap* content_settings_map() {
    return HostContentSettingsMapFactory::GetForProfile(&profile_);
  }

  sync_preferences::TestingPrefServiceSyncable* prefs() {
    return profile_.GetTestingPrefService();
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
};

TEST_F(BraveShieldsUtilTest, AreReferrersAllowedWithManagedPref) {
  prefs()->SetManagedPref(kManagedDefaultBraveReferrers,
                          base::Value(CONTENT_SETTING_ALLOW));
  EXPECT_TRUE(
      brave_shields::AreReferrersAllowed(content_settings_map(), GURL()));
  EXPECT_TRUE(brave_shields::AreReferrersAllowed(content_settings_map(),
                                                 GURL("http://brave.com")));

  prefs()->SetManagedPref(kManagedDefaultBraveReferrers,
                          base::Value(CONTENT_SETTING_BLOCK));
  EXPECT_FALSE(
      brave_shields::AreReferrersAllowed(content_settings_map(), GURL()));
  EXPECT_FALSE(brave_shields::AreReferrersAllowed(content_settings_map(),
                                                  GURL("http://brave.com")));
}
