/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_experiment_dto_user_data.h"

#include <string>

#include "bat/ads/internal/features/features.h"
#include "bat/ads/internal/unittest_base.h"
#include "bat/ads/internal/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsConfirmationExperimentDtoUserDataTest : public UnitTestBase {
 protected:
  BatAdsConfirmationExperimentDtoUserDataTest() = default;

  ~BatAdsConfirmationExperimentDtoUserDataTest() override = default;
};

TEST_F(BatAdsConfirmationExperimentDtoUserDataTest, ActiveExperiment) {
  // Arrange

  // Act
  base::Optional<std::string> study = features::GetStudy();

  // Assert
  std::string expected_study = "EpsilonGreedyBanditStudy";

  EXPECT_EQ(expected_study, study.value());
}

}  // namespace ads
