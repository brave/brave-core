/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/prefs/obsolete_pref_util.h"

#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_all_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kObsoleteOptedInToNotificationAds[] = "brave.brave_ads.enabled";

}  // namespace

class BraveAdsObsoletePrefUtilTest : public ::testing::Test {
 public:
  BraveAdsObsoletePrefUtilTest() {
    RegisterProfilePrefs(prefs_.registry());
    RegisterProfilePrefsForMigration(prefs_.registry());
  }

 protected:
  TestingPrefServiceSimple prefs_;
};

TEST_F(BraveAdsObsoletePrefUtilTest, MigrateOptedInToNotificationAdsPref) {
  // Arrange
  prefs_.SetBoolean(kObsoleteOptedInToNotificationAds, true);

  // Act
  MigrateObsoleteProfilePrefs(&prefs_);

  // Assert
  EXPECT_TRUE(prefs_.GetBoolean(prefs::kOptedInToNotificationAds));
  EXPECT_FALSE(prefs_.HasPrefPath(kObsoleteOptedInToNotificationAds));
}

TEST_F(BraveAdsObsoletePrefUtilTest, MigrateOptedOutOfNotificationAdsPref) {
  // Arrange
  prefs_.SetBoolean(kObsoleteOptedInToNotificationAds, false);

  // Act
  MigrateObsoleteProfilePrefs(&prefs_);

  // Assert
  EXPECT_FALSE(prefs_.GetBoolean(prefs::kOptedInToNotificationAds));
  EXPECT_FALSE(prefs_.HasPrefPath(kObsoleteOptedInToNotificationAds));
}

TEST_F(BraveAdsObsoletePrefUtilTest, MigrateUnsetNotificationAdsPref) {
  // Act
  MigrateObsoleteProfilePrefs(&prefs_);

  // Assert
  EXPECT_FALSE(prefs_.HasPrefPath(prefs::kOptedInToNotificationAds));
  EXPECT_FALSE(prefs_.GetBoolean(prefs::kOptedInToNotificationAds));
}

TEST_F(BraveAdsObsoletePrefUtilTest,
       PreserveOptedInToNotificationAdsPrefWhenMigratedMoreThanOnce) {
  // Arrange
  prefs_.SetBoolean(kObsoleteOptedInToNotificationAds, true);
  MigrateObsoleteProfilePrefs(&prefs_);

  // Act
  MigrateObsoleteProfilePrefs(&prefs_);

  // Assert
  EXPECT_TRUE(prefs_.GetBoolean(prefs::kOptedInToNotificationAds));
}

TEST_F(BraveAdsObsoletePrefUtilTest,
       PreserveOptedOutOfNotificationAdsPrefWhenMigratedMoreThanOnce) {
  // Arrange
  prefs_.SetBoolean(kObsoleteOptedInToNotificationAds, false);
  MigrateObsoleteProfilePrefs(&prefs_);

  // Act
  MigrateObsoleteProfilePrefs(&prefs_);

  // Assert
  EXPECT_FALSE(prefs_.GetBoolean(prefs::kOptedInToNotificationAds));
}

}  // namespace brave_ads
