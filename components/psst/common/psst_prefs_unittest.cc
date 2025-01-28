// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/psst/common/psst_prefs.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/psst/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace psst {

class PsstPrefsUnitTest : public testing::Test {
 public:
  PsstPrefsUnitTest() {
    feature_list_.InitAndEnableFeature(psst::features::kBravePsst);
    psst::RegisterProfilePrefs(pref_service_.registry());
  }

  PrefService* GetPrefs() { return &pref_service_; }

 private:
  base::test::ScopedFeatureList feature_list_;
  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

TEST_F(PsstPrefsUnitTest, TestPsstSettings) {
  auto* prefs = GetPrefs();
  SetPsstSettings("user1", "twitter", PsstSettings{psst::kAsk, 1}, prefs);
  SetPsstSettings("user2", "twitter", PsstSettings{psst::kAllow, 2}, prefs);
  SetPsstSettings("user1", "linkedin", PsstSettings{psst::kBlock, 3}, prefs);
  auto settings = psst::GetPsstSettings("user1", "twitter", prefs);
  EXPECT_TRUE(settings.has_value());
  EXPECT_EQ(settings->consent_status, kAsk);
  EXPECT_EQ(settings->script_version, 1);

  settings = psst::GetPsstSettings("user2", "twitter", prefs);
  EXPECT_TRUE(settings.has_value());
  EXPECT_EQ(settings->consent_status, kAllow);
  EXPECT_EQ(settings->script_version, 2);

  settings = psst::GetPsstSettings("user1", "linkedin", prefs);
  EXPECT_TRUE(settings.has_value());
  EXPECT_EQ(settings->consent_status, kBlock);
  EXPECT_EQ(settings->script_version, 3);
}

}  // namespace psst
