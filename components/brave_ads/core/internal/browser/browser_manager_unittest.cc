/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/browser/browser_manager.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsBrowserManagerTest : public BrowserManagerObserver,
                                   public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    BrowserManager::GetInstance().AddObserver(this);
  }

  void TearDown() override {
    BrowserManager::GetInstance().RemoveObserver(this);

    UnitTestBase::TearDown();
  }

  void OnBrowserDidBecomeActive() override {
    browser_did_become_active_ = true;
  }

  void OnBrowserDidResignActive() override {
    browser_did_resign_active_ = true;
  }

  void OnBrowserDidEnterForeground() override {
    browser_did_enter_foreground_ = true;
  }

  void OnBrowserDidEnterBackground() override {
    browser_did_enter_background_ = true;
  }

  void ResetObserver() {
    browser_did_become_active_ = false;
    browser_did_resign_active_ = false;
    browser_did_enter_foreground_ = false;
    browser_did_enter_background_ = false;
  }

  bool browser_did_become_active_ = false;
  bool browser_did_resign_active_ = false;
  bool browser_did_enter_foreground_ = false;
  bool browser_did_enter_background_ = false;
};

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidBecomeActive) {
  // Arrange

  // Act
  NotifyBrowserDidBecomeActive();

  // Assert
  EXPECT_TRUE(BrowserManager::GetInstance().IsBrowserActive());
  EXPECT_TRUE(browser_did_become_active_);
  EXPECT_FALSE(browser_did_resign_active_);
}

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidResignActive) {
  // Arrange
  NotifyBrowserDidBecomeActive();
  ResetObserver();

  // Act
  NotifyBrowserDidResignActive();

  // Assert
  EXPECT_FALSE(BrowserManager::GetInstance().IsBrowserActive());
  EXPECT_FALSE(browser_did_become_active_);
  EXPECT_TRUE(browser_did_resign_active_);
}

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidEnterForeground) {
  // Arrange

  // Act
  NotifyBrowserDidEnterForeground();

  // Assert
  EXPECT_TRUE(BrowserManager::GetInstance().IsBrowserInForeground());
  EXPECT_TRUE(browser_did_enter_foreground_);
  EXPECT_FALSE(browser_did_enter_background_);
}

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidEnterBackground) {
  // Arrange
  NotifyBrowserDidEnterForeground();
  ResetObserver();

  // Act
  NotifyBrowserDidEnterBackground();

  // Assert
  EXPECT_FALSE(BrowserManager::GetInstance().IsBrowserInForeground());
  EXPECT_FALSE(browser_did_enter_foreground_);
  EXPECT_TRUE(browser_did_enter_background_);
}

TEST_F(BraveAdsBrowserManagerTest,
       OnNotifyDidInitializeAdsWhenBrowserIsActive) {
  // Arrange

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(BrowserManager::GetInstance().IsBrowserActive());
  EXPECT_TRUE(BrowserManager::GetInstance().IsBrowserInForeground());
}

TEST_F(BraveAdsBrowserManagerTest,
       OnNotifyDidInitializeAdsWhenBrowserIsInactive) {
  // Arrange
  MockIsBrowserActive(ads_client_mock_, false);

  // Act
  NotifyDidInitializeAds();

  // Assert
  EXPECT_FALSE(BrowserManager::GetInstance().IsBrowserActive());
  EXPECT_FALSE(BrowserManager::GetInstance().IsBrowserInForeground());
}

}  // namespace brave_ads
