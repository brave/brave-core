/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/top_segment_user_data.h"

#include <memory>

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/resources/language_components_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/targeting/targeting_unittest_helper.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsTopSegmentUserDataTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    targeting_helper_ =
        std::make_unique<test::TargetingHelper>(task_environment_);

    NotifyDidUpdateResourceComponent(kLanguageComponentManifestVersion,
                                     kLanguageComponentId);
  }

  std::unique_ptr<test::TargetingHelper> targeting_helper_;
};

TEST_F(BraveAdsTopSegmentUserDataTest, BuildTopSegmentUserDataForRewardsUser) {
  // Arrange
  targeting_helper_->MockInterest();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/false);

  // Act
  const base::Value::Dict user_data = BuildTopSegmentUserData(transaction);

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "topSegment": [
                        {
                          "interest": "personal finance-banking"
                        }
                      ]
                    })"),
            user_data);
}

TEST_F(BraveAdsTopSegmentUserDataTest,
       DoNotBuildTopSegmentUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  targeting_helper_->MockInterest();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/false);

  // Act
  const base::Value::Dict user_data = BuildTopSegmentUserData(transaction);

  // Assert
  EXPECT_THAT(user_data, ::testing::IsEmpty());
}

TEST_F(BraveAdsTopSegmentUserDataTest,
       DoNotBuildTopSegmentUserDataForNonViewedConfirmationType) {
  // Arrange
  targeting_helper_->MockInterest();

  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd, ConfirmationType::kClicked,
      /*should_generate_random_uuids=*/false);

  // Act
  const base::Value::Dict user_data = BuildTopSegmentUserData(transaction);

  // Assert
  EXPECT_THAT(user_data, ::testing::IsEmpty());
}

TEST_F(BraveAdsTopSegmentUserDataTest,
       DoNotBuildTopSegmentUserDataIfNoTargeting) {
  // Arrange
  const TransactionInfo transaction = test::BuildUnreconciledTransaction(
      /*value=*/0.01, AdType::kNotificationAd,
      ConfirmationType::kViewedImpression,
      /*should_generate_random_uuids=*/false);

  // Act
  const base::Value::Dict user_data = BuildTopSegmentUserData(transaction);

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "topSegment": []
                    })"),
            user_data);
}

}  // namespace brave_ads
