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

  void OnRewardsPanelRequested(const mojom::RewardsPanelArgs& args) override {
    f_(args);
  }

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

    observer_ = MakePanelObserver([this](const mojom::RewardsPanelArgs& args) {
      last_args_ = args.Clone();
    });
    coordinator_->AddObserver(observer_.get());
  }

  void TearDown() override {
    // Clean up coordinator pointer, so it doesn't dangle during the browsers
    // destruction.
    coordinator_ = nullptr;

    BrowserWithTestWindowTest::TearDown();
  }

 protected:
  const mojom::RewardsPanelArgs& last_args() const {
    CHECK(last_args_);
    return *last_args_;
  }

  RewardsPanelCoordinator& coordinator() { return *coordinator_; }

 private:
  mojom::RewardsPanelArgsPtr last_args_;
  std::unique_ptr<RewardsPanelCoordinator::Observer> observer_;
  raw_ptr<RewardsPanelCoordinator> coordinator_ = nullptr;
};

TEST_F(RewardsPanelCoordinatorTest, OpenRewardsPanel) {
  EXPECT_TRUE(coordinator().OpenRewardsPanel());
  EXPECT_EQ(last_args().view, mojom::RewardsPanelView::kDefault);
  EXPECT_EQ(last_args().data, "");
}

TEST_F(RewardsPanelCoordinatorTest, ShowRewardsSetup) {
  EXPECT_TRUE(coordinator().ShowRewardsSetup());
  EXPECT_EQ(last_args().view, mojom::RewardsPanelView::kRewardsSetup);
  EXPECT_EQ(last_args().data, "");
}

TEST_F(RewardsPanelCoordinatorTest, ShowAdaptiveCaptcha) {
  EXPECT_TRUE(coordinator().ShowAdaptiveCaptcha());
  EXPECT_EQ(last_args().view, mojom::RewardsPanelView::kAdaptiveCaptcha);
  EXPECT_EQ(last_args().data, "");
}

}  // namespace brave_rewards
