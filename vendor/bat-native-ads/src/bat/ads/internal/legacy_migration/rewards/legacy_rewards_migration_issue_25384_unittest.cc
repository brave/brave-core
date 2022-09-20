/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/legacy_migration/rewards/legacy_rewards_migration.h"

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

using ::testing::_;

namespace {
constexpr char kIssue25384ConfirmationStateFilename[] =
    "confirmations_issue_25384.json";
}  // namespace

class BatAdsLegacyRewardsMigrationIssue25384Test : public UnitTestBase {
 protected:
  BatAdsLegacyRewardsMigrationIssue25384Test() = default;

  void SetUpMocks() override {
    CopyFileFromTestPathToTempPath(kIssue25384ConfirmationStateFilename,
                                   kConfirmationStateFilename);
  }
};

TEST_F(BatAdsLegacyRewardsMigrationIssue25384Test, Migrate) {
  // Arrange
  ads_client_mock_->SetBooleanPref(prefs::kHasMigratedRewardsState, false);

  EXPECT_CALL(*ads_client_mock_, Load(kConfirmationStateFilename, _));

  // Act
  rewards::Migrate([=](const bool success) { ASSERT_TRUE(success); });

  // Assert
  EXPECT_TRUE(
      ads_client_mock_->GetBooleanPref(prefs::kHasMigratedRewardsState));
}

}  // namespace ads
