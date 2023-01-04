/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/user_interaction/idle_detection/idle_detection_manager.h"

#include "base/time/time.h"
#include "bat/ads/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsIdleDetectionManagerTest : public IdleDetectionManagerObserver,
                                       public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    IdleDetectionManager::GetInstance()->AddObserver(this);
  }

  void TearDown() override {
    IdleDetectionManager::GetInstance()->RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnUserDidBecomeActive(const base::TimeDelta idle_time,
                             const bool screen_was_locked) override {
    user_is_active_ = true;
    idle_time_ = idle_time;
    screen_was_locked_ = screen_was_locked;
  }

  void OnUserDidBecomeIdle() override { user_is_active_ = false; }

  bool user_is_active_ = false;
  base::TimeDelta idle_time_;
  bool screen_was_locked_ = false;
};

TEST_F(BatAdsIdleDetectionManagerTest, HasInstance) {
  // Arrange

  // Act
  const bool has_instance = IdleDetectionManager::HasInstance();

  // Assert
  EXPECT_TRUE(has_instance);
}

TEST_F(BatAdsIdleDetectionManagerTest, UserDidBecomeActive) {
  // Arrange

  // Act
  IdleDetectionManager::GetInstance()->UserDidBecomeActive(
      /*idle_time */ base::Seconds(5), /*screen_was_locked*/ false);

  // Assert
  EXPECT_TRUE(user_is_active_);
  EXPECT_EQ(base::Seconds(5), idle_time_);
  EXPECT_FALSE(screen_was_locked_);
}

TEST_F(BatAdsIdleDetectionManagerTest,
       UserDidBecomeActiveWhileDeviceWasLocked) {
  // Arrange

  // Act
  IdleDetectionManager::GetInstance()->UserDidBecomeActive(
      /*idle_time */ base::Seconds(5), /*screen_was_locked*/ true);

  // Assert
  EXPECT_TRUE(user_is_active_);
  EXPECT_EQ(base::Seconds(5), idle_time_);
  EXPECT_TRUE(screen_was_locked_);
}

TEST_F(BatAdsIdleDetectionManagerTest, UserDidBecomeIdle) {
  // Arrange

  // Act
  IdleDetectionManager::GetInstance()->UserDidBecomeIdle();

  // Assert
  EXPECT_FALSE(user_is_active_);
}

}  // namespace ads
