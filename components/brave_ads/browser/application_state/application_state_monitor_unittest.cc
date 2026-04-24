/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/browser/application_state/application_state_monitor.h"

#include "brave/components/brave_ads/browser/application_state/test/test_application_state_observer.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsApplicationStateMonitorTest : public testing::Test,
                                            public ApplicationStateMonitor {
 protected:
  TestApplicationStateObserver application_state_observer_;
};

TEST_F(BraveAdsApplicationStateMonitorTest,
       IsBrowserActiveReturnsTrueByDefault) {
  // Act & Assert
  EXPECT_TRUE(IsBrowserActive());
}

TEST_F(BraveAdsApplicationStateMonitorTest,
       NotifiesObserverWhenBrowserEntersForeground) {
  // Arrange
  AddObserver(&application_state_observer_);

  // Act
  NotifyBrowserDidBecomeActive();

  // Assert
  EXPECT_EQ(1U, application_state_observer_.foreground_count());
  EXPECT_EQ(0U, application_state_observer_.background_count());

  RemoveObserver(&application_state_observer_);
}

TEST_F(BraveAdsApplicationStateMonitorTest,
       NotifiesObserverWhenBrowserEntersBackground) {
  // Arrange
  AddObserver(&application_state_observer_);

  // Act
  NotifyBrowserDidResignActive();

  // Assert
  EXPECT_EQ(0U, application_state_observer_.foreground_count());
  EXPECT_EQ(1U, application_state_observer_.background_count());

  RemoveObserver(&application_state_observer_);
}

TEST_F(BraveAdsApplicationStateMonitorTest, DoesNotNotifyObserverAfterRemoval) {
  // Arrange
  AddObserver(&application_state_observer_);
  RemoveObserver(&application_state_observer_);

  // Act
  NotifyBrowserDidBecomeActive();
  NotifyBrowserDidResignActive();

  // Assert
  EXPECT_EQ(0U, application_state_observer_.foreground_count());
  EXPECT_EQ(0U, application_state_observer_.background_count());
}

TEST_F(BraveAdsApplicationStateMonitorTest,
       NotifiesAllObserversOnForegroundTransition) {
  // Arrange
  TestApplicationStateObserver application_state_observer_1;
  AddObserver(&application_state_observer_1);
  TestApplicationStateObserver application_state_observer_2;
  AddObserver(&application_state_observer_2);

  // Act
  NotifyBrowserDidBecomeActive();

  // Assert
  EXPECT_EQ(1U, application_state_observer_1.foreground_count());
  EXPECT_EQ(1U, application_state_observer_2.foreground_count());

  RemoveObserver(&application_state_observer_1);
  RemoveObserver(&application_state_observer_2);
}

TEST_F(BraveAdsApplicationStateMonitorTest,
       NotifiesAllObserversOnBackgroundTransition) {
  // Arrange
  TestApplicationStateObserver application_state_observer_1;
  AddObserver(&application_state_observer_1);
  TestApplicationStateObserver application_state_observer_2;
  AddObserver(&application_state_observer_2);

  // Act
  NotifyBrowserDidResignActive();

  // Assert
  EXPECT_EQ(1U, application_state_observer_1.background_count());
  EXPECT_EQ(1U, application_state_observer_2.background_count());

  RemoveObserver(&application_state_observer_1);
  RemoveObserver(&application_state_observer_2);
}

}  // namespace brave_ads
