/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"

#include "chrome/browser/ui/browser_window/test/mock_browser_window_interface.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/test/mock_base_window.h"

namespace brave_rewards {

namespace {

class CountingPanelObserver : public RewardsPanelCoordinator::Observer {
 public:
  void OnRewardsPanelRequested() override { ++request_count_; }

  int request_count() const { return request_count_; }

 private:
  int request_count_ = 0;
};

}  // namespace

class RewardsPanelCoordinatorTest : public testing::Test {
 public:
  RewardsPanelCoordinatorTest() {
    ON_CALL(browser_window_, GetWindow())
        .WillByDefault(testing::Return(&window_));
    coordinator_.AddObserver(&observer_);
  }

 protected:
  // Declaration order matters: `coordinator_` must be destroyed before
  // `browser_window_`, which hosts its user data.
  testing::NiceMock<MockBrowserWindowInterface> browser_window_;
  testing::NiceMock<ui::MockBaseWindow> window_;
  CountingPanelObserver observer_;
  RewardsPanelCoordinator coordinator_{&browser_window_};
};

TEST_F(RewardsPanelCoordinatorTest, OpenRewardsPanel) {
  EXPECT_CALL(window_, IsMinimized()).WillOnce(testing::Return(false));
  EXPECT_CALL(window_, Activate()).Times(0);

  EXPECT_TRUE(coordinator_.OpenRewardsPanel());
  EXPECT_EQ(1, observer_.request_count());
}

TEST_F(RewardsPanelCoordinatorTest, OpenRewardsPanelActivatesMinimizedWindow) {
  EXPECT_CALL(window_, IsMinimized()).WillOnce(testing::Return(true));
  EXPECT_CALL(window_, Activate()).Times(1);

  EXPECT_TRUE(coordinator_.OpenRewardsPanel());
  EXPECT_EQ(1, observer_.request_count());
}

TEST_F(RewardsPanelCoordinatorTest, OpenRewardsPanelWithoutObserversFails) {
  coordinator_.RemoveObserver(&observer_);

  EXPECT_FALSE(coordinator_.OpenRewardsPanel());
  EXPECT_EQ(0, observer_.request_count());
}

}  // namespace brave_rewards
