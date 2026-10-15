/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/browser/mac/sparkle_glue.h"

#include "base/test/task_environment.h"
#import "brave/browser/mac/fake_sparkle.h"
#include "testing/gtest/include/gtest/gtest.h"

class SparkleGlueTest : public testing::Test {
 protected:
  void SetUp() override {
    glue_ = [[SparkleGlue alloc] init];
    updater_ = [[FakeSUUpdater alloc] init];
    updater_.driver = [[FakeSUUpdateDriver alloc] init];
  }

  // Sparkle's callback posts a status update to the thread pool.
  void TearDown() override { task_environment_.RunUntilIdle(); }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::MainThreadType::UI};
  SparkleGlue* __strong glue_;
  FakeSUUpdater* __strong updater_;
};

// A downloaded update is installed by Sparkle, which then relaunches the new
// version.
TEST_F(SparkleGlueTest, RelaunchInstallsPendingUpdate) {
  SimulateSparkleUpdateWillBeInstalledOnQuit(glue_, updater_);

  EXPECT_TRUE([glue_ relaunch]);
  EXPECT_EQ(1, updater_.driver.installCount);
  EXPECT_TRUE(updater_.driver.lastRelaunch);
  EXPECT_FALSE(updater_.driver.lastShowUI);
}

TEST_F(SparkleGlueTest, RelaunchWithoutPendingUpdateLeavesItToChromium) {
  [glue_ setUpdaterForTesting:updater_];

  EXPECT_FALSE([glue_ relaunch]);
  EXPECT_EQ(0, updater_.driver.installCount);
}

TEST_F(SparkleGlueTest, RelaunchWithoutDriverLeavesItToChromium) {
  SimulateSparkleUpdateWillBeInstalledOnQuit(glue_, updater_);
  FakeSUUpdateDriver* driver = updater_.driver;
  updater_.driver = nil;

  EXPECT_FALSE([glue_ relaunch]);
  EXPECT_EQ(0, driver.installCount);
}
