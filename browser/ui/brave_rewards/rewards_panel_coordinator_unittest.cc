/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_rewards/rewards_panel_coordinator.h"

#include <memory>
#include <string>
#include <utility>

#include "chrome/test/base/browser_with_test_window_test.h"

namespace brave_rewards {

namespace {

template <typename F>
class PanelObserver : public RewardsPanelCoordinator::Observer {
 public:
  explicit PanelObserver(F f) : f_(std::move(f)) {}
  ~PanelObserver() override = default;

  void OnRewardsPanelRequested() override { f_(); }

 private:
  F f_;
};

template <typename F>
auto MakePanelObserver(F&& f) {
  return std::make_unique<PanelObserver<F>>(std::forward<F>(f));
}

}  // namespace

class RewardsPanelCoordinatorTest : public BrowserWithTestWindowTest {
 public:
  void SetUp() override {
    BrowserWithTestWindowTest::SetUp();

    RewardsPanelCoordinator::CreateForBrowser(browser());
    coordinator_ = RewardsPanelCoordinator::FromBrowser(browser());
    DCHECK(coordinator_);

    observer_ = MakePanelObserver([this]() { called_ = true; });
    coordinator_->AddObserver(observer_.get());
  }

  void TearDown() override {
    // Clean up coordinator pointer, so it doesn't dangle during the browsers
    // destruction.
    coordinator_ = nullptr;

    BrowserWithTestWindowTest::TearDown();
  }

 protected:
  bool called() const { return called_; }

  RewardsPanelCoordinator& coordinator() { return *coordinator_; }

 private:
  bool called_ = false;
  std::unique_ptr<RewardsPanelCoordinator::Observer> observer_;
  raw_ptr<RewardsPanelCoordinator> coordinator_ = nullptr;
};

TEST_F(RewardsPanelCoordinatorTest, OpenRewardsPanel) {
  EXPECT_TRUE(coordinator().OpenRewardsPanel());
  EXPECT_TRUE(called());
}

}  // namespace brave_rewards
