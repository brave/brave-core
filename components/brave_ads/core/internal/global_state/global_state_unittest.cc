/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/internal/ads_client_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

using ::testing::NiceMock;

class BraveAdsGlobalStateTest : public ::testing::Test {
 protected:
  void SetUp() override {
    global_state_ = std::make_unique<GlobalState>(&ads_client_mock_);
  }

 protected:
  NiceMock<AdsClientMock> ads_client_mock_;

  std::unique_ptr<GlobalState> global_state_;
};

TEST_F(BraveAdsGlobalStateTest, HasInstance) {
  // Arrange

  // Act
  const bool has_instance = GlobalState::HasInstance();

  // Assert
  EXPECT_TRUE(has_instance);
}

TEST_F(BraveAdsGlobalStateTest, HasNoInstanceAfterDestruction) {
  // Arrange
  global_state_.reset();

  // Act
  const bool has_instance = GlobalState::HasInstance();

  // Assert
  EXPECT_FALSE(has_instance);
}

TEST_F(BraveAdsGlobalStateTest, CheckManagersNotNull) {
  // Arrange

  // Act

  // Assert
  EXPECT_TRUE(GlobalState::GetInstance()->GetAdsClient());
  EXPECT_TRUE(GlobalState::GetInstance()->GetBrowserManager());
  EXPECT_TRUE(GlobalState::GetInstance()->GetClientStateManager());
  EXPECT_TRUE(GlobalState::GetInstance()->GetConfirmationStateManager());
  EXPECT_TRUE(GlobalState::GetInstance()->GetDatabaseManager());
  EXPECT_TRUE(GlobalState::GetInstance()->GetDiagnosticManager());
  EXPECT_TRUE(GlobalState::GetInstance()->GetHistoryManager());
  EXPECT_TRUE(GlobalState::GetInstance()->GetNotificationAdManager());
  EXPECT_TRUE(GlobalState::GetInstance()->GetPredictorsManager());
  EXPECT_TRUE(GlobalState::GetInstance()->GetTabManager());
  EXPECT_TRUE(GlobalState::GetInstance()->GetUserActivityManager());
}

}  // namespace brave_ads
