// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/speedreader/speedreader_service.h"

#include "brave/browser/speedreader/speedreader_service_factory.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace speedreader {

class SpeedreaderServiceTest : public testing::Test {
 public:
  SpeedreaderService* speedreader_service() {
    return SpeedreaderServiceFactory::GetForBrowserContext(&profile_);
  }

  TestingProfile* profile() { return &profile_; }

 private:
  content::BrowserTaskEnvironment task_environment_;
  TestingProfile profile_;
};

TEST_F(SpeedreaderServiceTest, CheckPolicyDefault) {
  EXPECT_FALSE(profile()->GetPrefs()->GetBoolean(kSpeedreaderDisabledByPolicy));
  EXPECT_TRUE(speedreader_service());
}

TEST_F(SpeedreaderServiceTest, CheckPolicyEnabled) {
  profile()->GetPrefs()->SetBoolean(kSpeedreaderDisabledByPolicy, true);
  EXPECT_FALSE(speedreader_service());
}

TEST_F(SpeedreaderServiceTest, CheckPolicyDisabled) {
  profile()->GetPrefs()->SetBoolean(kSpeedreaderDisabledByPolicy, false);
  EXPECT_TRUE(speedreader_service());
}

TEST_F(SpeedreaderServiceTest, CheckDefaultContentSettings) {
  EXPECT_TRUE(speedreader_service()->IsFeatureEnabled());
  EXPECT_FALSE(speedreader_service()->IsEnabledForAllSites());

  for (const bool enabled : {true, false}) {
    speedreader_service()->EnableForAllSites(enabled);
    EXPECT_EQ(enabled, speedreader_service()->IsEnabledForAllSites());
  }
}

TEST_F(SpeedreaderServiceTest, CheckDefaultContentSettingsByPref) {
  profile()->GetPrefs()->SetBoolean(kSpeedreaderPrefEnabledForAllSites, true);
  EXPECT_TRUE(speedreader_service()->IsFeatureEnabled());
  EXPECT_TRUE(speedreader_service()->IsEnabledForAllSites());

  for (const bool enabled : {true, false}) {
    speedreader_service()->EnableForAllSites(enabled);
    EXPECT_EQ(enabled, speedreader_service()->IsEnabledForAllSites());
  }
}

TEST_F(SpeedreaderServiceTest, ChangingPref) {
  EXPECT_TRUE(speedreader_service()->IsFeatureEnabled());
  EXPECT_FALSE(speedreader_service()->IsEnabledForAllSites());

  for (const bool enabled : {true, false}) {
    profile()->GetPrefs()->SetBoolean(kSpeedreaderPrefEnabledForAllSites,
                                      enabled);
    EXPECT_EQ(enabled, speedreader_service()->IsEnabledForAllSites());
  }
}

TEST_F(SpeedreaderServiceTest, ChangingContentSettings) {
  EXPECT_TRUE(speedreader_service()->IsFeatureEnabled());
  EXPECT_FALSE(speedreader_service()->IsEnabledForAllSites());

  for (const bool enabled : {true, false}) {
    speedreader_service()->EnableForAllSites(enabled);
    EXPECT_EQ(enabled, profile()->GetPrefs()->GetBoolean(
                           kSpeedreaderPrefEnabledForAllSites));
  }
}

TEST_F(SpeedreaderServiceTest, SiteSettings) {
  const GURL site("https://example.com");

  for (const bool enabled : {true, false}) {
    speedreader_service()->EnableForAllSites(enabled);
    EXPECT_EQ(enabled, speedreader_service()->IsEnabledForSite(site));
    EXPECT_FALSE(speedreader_service()->IsExplicitlyEnabledForSite(site));
    EXPECT_FALSE(speedreader_service()->IsExplicitlyDisabledForSite(site));
  }

  speedreader_service()->EnableForAllSites(false);
  for (const bool enabled : {true, false}) {
    speedreader_service()->EnableForSite(site, enabled);
    EXPECT_EQ(enabled, speedreader_service()->IsEnabledForSite(site));
    EXPECT_EQ(enabled, speedreader_service()->IsExplicitlyEnabledForSite(site));
    EXPECT_EQ(!enabled,
              speedreader_service()->IsExplicitlyDisabledForSite(site));
  }
}

}  // namespace speedreader
