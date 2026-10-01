/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ntp_background_images/common/view_counter_pref_registry.h"

#include "brave/components/ntp_background_images/common/new_tab_takeover_disclosure_constants.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "build/build_config.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ntp_background_images {

class ViewCounterPrefRegistryMigrationTest : public testing::Test {
 public:
  ViewCounterPrefRegistryMigrationTest() {
    RegisterProfilePrefs(pref_service_.registry());
  }

  ~ViewCounterPrefRegistryMigrationTest() override = default;

  PrefService* pref_service() { return &pref_service_; }

 private:
  sync_preferences::TestingPrefServiceSyncable pref_service_;
};

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

TEST_F(ViewCounterPrefRegistryMigrationTest,
       MigratesTheInfobarRemainingDisplayCountToTheDisclosureTooltipCount) {
  pref_service()->SetInteger(prefs::kNewTabTakeoverInfobarRemainingDisplayCount,
                             3);

  MigrateObsoleteProfilePrefs(pref_service());

  EXPECT_FALSE(pref_service()->HasPrefPath(
      prefs::kNewTabTakeoverInfobarRemainingDisplayCount));
  EXPECT_EQ(3,
            pref_service()->GetInteger(
                prefs::kNewTabTakeoverDisclosureTooltipRemainingDisplayCount));
}

TEST_F(ViewCounterPrefRegistryMigrationTest,
       DoesNotTouchTheDisclosureTooltipCountWhenTheInfobarPrefWasNeverSet) {
  MigrateObsoleteProfilePrefs(pref_service());

  EXPECT_FALSE(pref_service()->HasPrefPath(
      prefs::kNewTabTakeoverInfobarRemainingDisplayCount));
  EXPECT_EQ(kNewTabTakeoverDisclosureTooltipRemainingDisplayCountThreshold,
            pref_service()->GetInteger(
                prefs::kNewTabTakeoverDisclosureTooltipRemainingDisplayCount));
}

#else

TEST_F(ViewCounterPrefRegistryMigrationTest,
       DoesNotMigrateTheInfobarRemainingDisplayCount) {
  pref_service()->SetInteger(prefs::kNewTabTakeoverInfobarRemainingDisplayCount,
                             3);

  MigrateObsoleteProfilePrefs(pref_service());

  EXPECT_EQ(3, pref_service()->GetInteger(
                   prefs::kNewTabTakeoverInfobarRemainingDisplayCount));
  EXPECT_EQ(kNewTabTakeoverDisclosureTooltipRemainingDisplayCountThreshold,
            pref_service()->GetInteger(
                prefs::kNewTabTakeoverDisclosureTooltipRemainingDisplayCount));
}

#endif  // !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)

}  // namespace ntp_background_images
