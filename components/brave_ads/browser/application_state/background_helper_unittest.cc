/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/application_state/background_helper.h"

#include <cstddef>

#include "brave/components/brave_ads/browser/application_state/background_helper_observer.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

class TestBackgroundHelperObserver : public BackgroundHelperObserver {
 public:
  void OnBrowserDidEnterForeground() override { ++foreground_count_; }
  void OnBrowserDidEnterBackground() override { ++background_count_; }

  size_t foreground_count() const { return foreground_count_; }
  size_t background_count() const { return background_count_; }

 private:
  size_t foreground_count_ = 0;
  size_t background_count_ = 0;
};

}  // namespace

class BraveAdsBackgroundHelperTest : public testing::Test {
 protected:
  BackgroundHelper background_helper_;
  TestBackgroundHelperObserver background_helper_observer_;
};

TEST_F(BraveAdsBackgroundHelperTest, IsInForegroundReturnsTrueByDefault) {
  // Act & Assert
  EXPECT_TRUE(background_helper_.IsInForeground());
}

TEST_F(BraveAdsBackgroundHelperTest,
       NotifiesObserverWhenBrowserEntersForeground) {
  // Arrange
  background_helper_.AddObserver(&background_helper_observer_);

  // Act
  background_helper_.NotifyDidEnterForeground();

  // Assert
  EXPECT_EQ(1U, background_helper_observer_.foreground_count());
  EXPECT_EQ(0U, background_helper_observer_.background_count());

  background_helper_.RemoveObserver(&background_helper_observer_);
}

TEST_F(BraveAdsBackgroundHelperTest,
       NotifiesObserverWhenBrowserEntersBackground) {
  // Arrange
  background_helper_.AddObserver(&background_helper_observer_);

  // Act
  background_helper_.NotifyDidEnterBackground();

  // Assert
  EXPECT_EQ(0U, background_helper_observer_.foreground_count());
  EXPECT_EQ(1U, background_helper_observer_.background_count());

  background_helper_.RemoveObserver(&background_helper_observer_);
}

TEST_F(BraveAdsBackgroundHelperTest, DoesNotNotifyObserverAfterRemoval) {
  // Arrange
  background_helper_.AddObserver(&background_helper_observer_);
  background_helper_.RemoveObserver(&background_helper_observer_);

  // Act
  background_helper_.NotifyDidEnterForeground();
  background_helper_.NotifyDidEnterBackground();

  // Assert
  EXPECT_EQ(0U, background_helper_observer_.foreground_count());
  EXPECT_EQ(0U, background_helper_observer_.background_count());
}

TEST_F(BraveAdsBackgroundHelperTest,
       NotifiesAllObserversOnForegroundTransition) {
  // Arrange
  TestBackgroundHelperObserver background_helper_observer_1;
  background_helper_.AddObserver(&background_helper_observer_1);
  TestBackgroundHelperObserver background_helper_observer_2;
  background_helper_.AddObserver(&background_helper_observer_2);

  // Act
  background_helper_.NotifyDidEnterForeground();

  // Assert
  EXPECT_EQ(1U, background_helper_observer_1.foreground_count());
  EXPECT_EQ(1U, background_helper_observer_2.foreground_count());

  background_helper_.RemoveObserver(&background_helper_observer_1);
  background_helper_.RemoveObserver(&background_helper_observer_2);
}

TEST_F(BraveAdsBackgroundHelperTest,
       NotifiesAllObserversOnBackgroundTransition) {
  // Arrange
  TestBackgroundHelperObserver background_helper_observer_1;
  background_helper_.AddObserver(&background_helper_observer_1);
  TestBackgroundHelperObserver background_helper_observer_2;
  background_helper_.AddObserver(&background_helper_observer_2);

  // Act
  background_helper_.NotifyDidEnterBackground();

  // Assert
  EXPECT_EQ(1U, background_helper_observer_1.background_count());
  EXPECT_EQ(1U, background_helper_observer_2.background_count());

  background_helper_.RemoveObserver(&background_helper_observer_1);
  background_helper_.RemoveObserver(&background_helper_observer_2);
}

}  // namespace brave_ads
