/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_default_provider.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/browser/website_settings_registry.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content_settings {

class BraveContentSettingsDefaultProviderTest : public testing::Test {
 public:
  BraveContentSettingsDefaultProviderTest()
      : provider_(profile_.GetPrefs(), false) {}
  ~BraveContentSettingsDefaultProviderTest() override {
    provider_.ShutdownOnUIThread();
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
  BraveDefaultProvider provider_;
};

TEST_F(BraveContentSettingsDefaultProviderTest, DiscardObsoleteAutoplayAsk) {
  PrefService* prefs = profile_.GetPrefs();
  const std::string& autoplay_pref_path =
      WebsiteSettingsRegistry::GetInstance()
          ->Get(ContentSettingsType::AUTOPLAY)
          ->default_value_pref_name();

  // The ASK value of the autoplay content setting should be discarded.
  {
    prefs->SetInteger(autoplay_pref_path, CONTENT_SETTING_ASK);
    BraveDefaultProvider provider(prefs, false);
    EXPECT_FALSE(prefs->HasPrefPath(autoplay_pref_path));
  }

  // Other values of the autoplay content setting should be preserved.
  {
    prefs->SetInteger(autoplay_pref_path, CONTENT_SETTING_ALLOW);
    BraveDefaultProvider provider(prefs, false);
    EXPECT_TRUE(prefs->HasPrefPath(autoplay_pref_path));
    EXPECT_EQ(CONTENT_SETTING_ALLOW, prefs->GetInteger(autoplay_pref_path));
  }

  {
    prefs->SetInteger(autoplay_pref_path, CONTENT_SETTING_BLOCK);
    BraveDefaultProvider provider(prefs, false);

    EXPECT_TRUE(prefs->HasPrefPath(autoplay_pref_path));
    EXPECT_EQ(CONTENT_SETTING_BLOCK, prefs->GetInteger(autoplay_pref_path));
  }
}

}  // namespace content_settings
