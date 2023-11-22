/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/mock_callback.h"

#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_profile_pref_value.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/rewards/legacy_rewards_migration.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {
constexpr char kIssue25384ConfirmationStateFilename[] =
    "confirmations_issue_25384.json";
}  // namespace

class BraveAdsLegacyRewardsMigrationIssue25384Test : public UnitTestBase {
 protected:
  void SetUpMocks() override {
    ASSERT_TRUE(CopyFileFromTestPathToTempPath(
        kIssue25384ConfirmationStateFilename, kConfirmationStateFilename));
  }
};

TEST_F(BraveAdsLegacyRewardsMigrationIssue25384Test, Migrate) {
  // Arrange
  SetProfileBooleanPrefValue(prefs::kHasMigratedRewardsState, false);

  // Act & Assert
  base::MockCallback<InitializeCallback> callback;
  EXPECT_CALL(callback, Run(/*success=*/true));
  rewards::Migrate(callback.Get());

  EXPECT_TRUE(GetProfileBooleanPref(prefs::kHasMigratedRewardsState));
}

}  // namespace brave_ads
