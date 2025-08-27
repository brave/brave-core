// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/upgrade_when_idle/upgrade_when_idle.h"

#include "base/test/mock_callback.h"
#include "base/test/task_environment.h"
#include "chrome/browser/first_run/scoped_relaunch_chrome_browser_override.h"
#include "chrome/browser/first_run/upgrade_util.h"
#include "chrome/browser/ui/browser_list.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/idle/scoped_set_idle_state.h"

namespace brave {

class UpgradeWhenIdleTest : public testing::Test {
 public:
  UpgradeWhenIdleTest() {
    mock_relaunch_callback_ = std::make_unique<::testing::StrictMock<
        base::MockCallback<upgrade_util::RelaunchChromeBrowserCallback>>>();
    relaunch_chrome_override_ =
        std::make_unique<upgrade_util::ScopedRelaunchChromeBrowserOverride>(
            mock_relaunch_callback_->Get());
  }

  void SetUp() override {
    upgrade_when_idle_ = std::make_unique<UpgradeWhenIdle>();
  }

 protected:
  void RunImplementation() {
    upgrade_when_idle_->OnUpgradeRecommended();
    task_environment_.FastForwardBy(base::Minutes(3));
    task_environment_.RunUntilIdle();
  }

  void ExpectUpgrade() { EXPECT_CALL(*mock_relaunch_callback_, Run); }

  std::unique_ptr<UpgradeWhenIdle> upgrade_when_idle_;
  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::TimeSource::MOCK_TIME};
  std::unique_ptr<::testing::StrictMock<
      base::MockCallback<upgrade_util::RelaunchChromeBrowserCallback>>>
      mock_relaunch_callback_;
  std::unique_ptr<upgrade_util::ScopedRelaunchChromeBrowserOverride>
      relaunch_chrome_override_;
};

TEST_F(UpgradeWhenIdleTest, UpgradeWhenIdle) {
  ui::ScopedSetIdleState idle(ui::IDLE_STATE_IDLE);
  ExpectUpgrade();
  RunImplementation();
}

TEST_F(UpgradeWhenIdleTest, UpgradeWhenLocked) {
  ui::ScopedSetIdleState locked(ui::IDLE_STATE_LOCKED);
  ExpectUpgrade();
  RunImplementation();
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenActive) {
  ui::ScopedSetIdleState active(ui::IDLE_STATE_ACTIVE);
  RunImplementation();
}

TEST_F(UpgradeWhenIdleTest, NoUpgradeWhenStateUnknown) {
  ui::ScopedSetIdleState unknown(ui::IDLE_STATE_UNKNOWN);
  RunImplementation();
}

}  // namespace brave
