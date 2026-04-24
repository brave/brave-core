/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Tests for MigrateObsoleteProfilePrefs.

#include "brave/components/brave_ads/core/public/prefs/obsolete_pref_util.h"

#include <string_view>

#include "base/values.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAdsObsoletePrefUtil*

namespace brave_ads {

namespace {

constexpr std::string_view kObsoleteShouldShowSearchResultAdClickedInfoBar =
    "brave.brave_ads.should_show_search_result_ad_clicked_infobar";

}  // namespace

class BraveAdsObsoletePrefUtilTest : public ::testing::Test {
 public:
  void SetUp() override { RegisterProfilePrefsForMigration(prefs_.registry()); }

 protected:
  TestingPrefServiceSimple prefs_;
};

TEST_F(BraveAdsObsoletePrefUtilTest,
       MigrateObsoleteProfilePrefsWhenRewardsIsDisabledByPolicy) {
  prefs_.registry()->RegisterBooleanPref(
      brave_rewards::prefs::kDisabledByPolicy, false);
  prefs_.registry()->RegisterBooleanPref(prefs::kEnabledByPolicy, true);
  prefs_.SetManagedPref(brave_rewards::prefs::kDisabledByPolicy,
                        base::Value(true));

  MigrateObsoleteProfilePrefs(&prefs_);

  EXPECT_FALSE(prefs_.GetBoolean(prefs::kEnabledByPolicy));
}

TEST_F(
    BraveAdsObsoletePrefUtilTest,
    MigrateObsoleteProfilePrefsDoesNotChangeAdsEnabledWhenRewardsIsManagedEnabled) {
  prefs_.registry()->RegisterBooleanPref(
      brave_rewards::prefs::kDisabledByPolicy, false);
  prefs_.registry()->RegisterBooleanPref(prefs::kEnabledByPolicy, true);
  prefs_.SetManagedPref(brave_rewards::prefs::kDisabledByPolicy,
                        base::Value(false));

  MigrateObsoleteProfilePrefs(&prefs_);

  EXPECT_TRUE(prefs_.GetBoolean(prefs::kEnabledByPolicy));
}

TEST_F(
    BraveAdsObsoletePrefUtilTest,
    MigrateObsoleteProfilePrefsDoesNotChangeAdsEnabledWhenRewardsIsNotManaged) {
  prefs_.registry()->RegisterBooleanPref(prefs::kEnabledByPolicy, true);

  MigrateObsoleteProfilePrefs(&prefs_);

  EXPECT_TRUE(prefs_.GetBoolean(prefs::kEnabledByPolicy));
}

TEST_F(BraveAdsObsoletePrefUtilTest,
       MigrateObsoleteProfilePrefsDoesNotMigrateWhenAdsEnabledAlreadySet) {
  prefs_.registry()->RegisterBooleanPref(
      brave_rewards::prefs::kDisabledByPolicy, false);
  prefs_.registry()->RegisterBooleanPref(prefs::kEnabledByPolicy, true);
  prefs_.SetManagedPref(brave_rewards::prefs::kDisabledByPolicy,
                        base::Value(true));
  prefs_.SetBoolean(prefs::kEnabledByPolicy, true);

  MigrateObsoleteProfilePrefs(&prefs_);

  EXPECT_TRUE(prefs_.GetBoolean(prefs::kEnabledByPolicy));
}

TEST_F(
    BraveAdsObsoletePrefUtilTest,
    MigrateObsoleteProfilePrefsMigratesShouldShowSearchResultAdClickedInfoBar) {
  prefs_.registry()->RegisterBooleanPref(
      prefs::kShouldShowSearchResultAdClickedInfoBar, false);
  prefs_.SetBoolean(kObsoleteShouldShowSearchResultAdClickedInfoBar, true);

  MigrateObsoleteProfilePrefs(&prefs_);

  EXPECT_FALSE(
      prefs_.HasPrefPath(kObsoleteShouldShowSearchResultAdClickedInfoBar));
}

TEST_F(
    BraveAdsObsoletePrefUtilTest,
    MigrateObsoleteProfilePrefsDoesNotMigrateShouldShowSearchResultAdClickedInfoBarWhenAbsent) {
  MigrateObsoleteProfilePrefs(&prefs_);

  EXPECT_FALSE(
      prefs_.HasPrefPath(kObsoleteShouldShowSearchResultAdClickedInfoBar));
}

TEST_F(BraveAdsObsoletePrefUtilTest,
       MigrateObsoleteProfilePrefsClearsP2APrefs) {
  constexpr std::string_view kP2APrefPath =
      "brave.weekly_storage.Brave.P2A.ad_notification.opportunities";
  prefs_.SetList(kP2APrefPath, base::ListValue().Append(1));

  MigrateObsoleteProfilePrefs(&prefs_);

  EXPECT_FALSE(prefs_.HasPrefPath(kP2APrefPath));
}

}  // namespace brave_ads
