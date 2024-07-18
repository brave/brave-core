/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/application_state/browser_manager.h"

#include "brave/components/brave_ads/core/internal/application_state/browser_manager_observer_mock.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsBrowserManagerTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    BrowserManager::GetInstance().AddObserver(&browser_manager_observer_mock_);
  }

  void TearDown() override {
    BrowserManager::GetInstance().RemoveObserver(
        &browser_manager_observer_mock_);

    test::TestBase::TearDown();
  }

  BrowserManagerObserverMock browser_manager_observer_mock_;
};

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidBecomeActive) {
  // Arrange
  EXPECT_CALL(browser_manager_observer_mock_, OnBrowserDidBecomeActive);

  // Act
  NotifyBrowserDidBecomeActive();

  // Assert
  EXPECT_TRUE(BrowserManager::GetInstance().IsActive());
}

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidResignActive) {
  // Arrange
  NotifyBrowserDidBecomeActive();

  EXPECT_CALL(browser_manager_observer_mock_, OnBrowserDidResignActive);

  // Act
  NotifyBrowserDidResignActive();

  // Assert
  EXPECT_FALSE(BrowserManager::GetInstance().IsActive());
}

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidEnterForeground) {
  // Arrange
  EXPECT_CALL(browser_manager_observer_mock_, OnBrowserDidEnterForeground);

  // Act
  NotifyBrowserDidEnterForeground();

  // Assert
  EXPECT_TRUE(BrowserManager::GetInstance().IsInForeground());
}

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidEnterBackground) {
  // Arrange
  NotifyBrowserDidEnterForeground();

  EXPECT_CALL(browser_manager_observer_mock_, OnBrowserDidEnterBackground);

  // Act
  NotifyBrowserDidEnterBackground();

  // Assert
  EXPECT_FALSE(BrowserManager::GetInstance().IsInForeground());
}

TEST_F(BraveAdsBrowserManagerTest,
       OnNotifyDidInitializeAdsWhenBrowserIsActive) {
  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(BrowserManager::GetInstance().IsInForeground());
}

TEST_F(BraveAdsBrowserManagerTest,
       OnNotifyDidInitializeAdsWhenBrowserIsInactive) {
  // Arrange
  test::MockIsBrowserActive(ads_client_mock_, false);

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_FALSE(BrowserManager::GetInstance().IsInForeground());
}

}  // namespace brave_ads
