/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/browser/browser_manager.h"

#include "brave/components/brave_ads/core/internal/browser/browser_manager_observer_mock.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsBrowserManagerTest : public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    BrowserManager::GetInstance().AddObserver(&observer_mock_);
  }

  void TearDown() override {
    BrowserManager::GetInstance().RemoveObserver(&observer_mock_);

    UnitTestBase::TearDown();
  }

  BrowserManagerObserverMock observer_mock_;
};

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidBecomeActive) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnBrowserDidBecomeActive);

  // Act
  NotifyBrowserDidBecomeActive();

  // Assert
  EXPECT_TRUE(BrowserManager::GetInstance().IsActive());
}

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidResignActive) {
  // Arrange
  NotifyBrowserDidBecomeActive();

  EXPECT_CALL(observer_mock_, OnBrowserDidResignActive);

  // Act
  NotifyBrowserDidResignActive();

  // Assert
  EXPECT_FALSE(BrowserManager::GetInstance().IsActive());
}

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidEnterForeground) {
  // Arrange
  EXPECT_CALL(observer_mock_, OnBrowserDidEnterForeground);

  // Act
  NotifyBrowserDidEnterForeground();

  // Assert
  EXPECT_TRUE(BrowserManager::GetInstance().IsInForeground());
}

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidEnterBackground) {
  // Arrange
  NotifyBrowserDidEnterForeground();

  EXPECT_CALL(observer_mock_, OnBrowserDidEnterBackground);

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
  EXPECT_TRUE(BrowserManager::GetInstance().IsActive());
  EXPECT_TRUE(BrowserManager::GetInstance().IsInForeground());
}

TEST_F(BraveAdsBrowserManagerTest,
       OnNotifyDidInitializeAdsWhenBrowserIsInactive) {
  // Arrange
  MockIsBrowserActive(ads_client_mock_, false);

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_FALSE(BrowserManager::GetInstance().IsActive());
  EXPECT_FALSE(BrowserManager::GetInstance().IsInForeground());
}

}  // namespace brave_ads
