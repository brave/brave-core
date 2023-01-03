/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/browser/browser_manager.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsBrowserManagerTest : public BrowserManagerObserver,
                                 public UnitTestBase {
 protected:
  void SetUp() override {
    UnitTestBase::SetUp();

    BrowserManager::GetInstance()->AddObserver(this);
  }

  void TearDown() override {
    BrowserManager::GetInstance()->RemoveObserver(this);

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

  bool browser_did_become_active_ = false;
  bool browser_did_resign_active_ = false;

  bool browser_did_enter_foreground_ = false;
  bool browser_did_enter_background_ = false;
};

TEST_F(BatAdsBrowserManagerTest, HasInstance) {
  // Arrange

  // Act
  const bool has_instance = BrowserManager::HasInstance();

  // Assert
  EXPECT_TRUE(has_instance);
}

TEST_F(BatAdsBrowserManagerTest, BrowserDidBecomeActive) {
  // Arrange
  BrowserManager::GetInstance()->SetBrowserIsInForeground(true);
  BrowserManager::GetInstance()->SetBrowserIsActive(false);

  // Act
  BrowserManager::GetInstance()->OnBrowserDidBecomeActive();

  // Assert
  EXPECT_TRUE(BrowserManager::GetInstance()->IsBrowserActive());
  EXPECT_TRUE(browser_did_become_active_);
  EXPECT_FALSE(browser_did_resign_active_);
}

TEST_F(BatAdsBrowserManagerTest, BrowserDidResignActive) {
  // Arrange
  BrowserManager::GetInstance()->SetBrowserIsActive(true);

  // Act
  BrowserManager::GetInstance()->OnBrowserDidResignActive();

  // Assert
  EXPECT_FALSE(BrowserManager::GetInstance()->IsBrowserActive());
  EXPECT_FALSE(browser_did_become_active_);
  EXPECT_TRUE(browser_did_resign_active_);
}

TEST_F(BatAdsBrowserManagerTest, BrowserDidEnterForeground) {
  // Arrange
  BrowserManager::GetInstance()->SetBrowserIsInForeground(false);

  // Act
  BrowserManager::GetInstance()->OnBrowserDidEnterForeground();

  // Assert
  EXPECT_TRUE(BrowserManager::GetInstance()->IsBrowserInForeground());
  EXPECT_TRUE(browser_did_enter_foreground_);
  EXPECT_FALSE(browser_did_enter_background_);
}

TEST_F(BatAdsBrowserManagerTest, BrowserDidEnterBackground) {
  // Arrange
  BrowserManager::GetInstance()->SetBrowserIsInForeground(true);

  // Act
  BrowserManager::GetInstance()->OnBrowserDidEnterBackground();

  // Assert
  EXPECT_FALSE(BrowserManager::GetInstance()->IsBrowserInForeground());
  EXPECT_FALSE(browser_did_enter_foreground_);
  EXPECT_TRUE(browser_did_enter_background_);
}

}  // namespace ads
