/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/browser_manager/browser_manager.h"

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsBrowserManagerTest : public BrowserManagerObserver,
                                 public UnitTestBase {
 protected:
  BatAdsBrowserManagerTest() = default;

  ~BatAdsBrowserManagerTest() override = default;

  void SetUp() override {
    UnitTestBase::SetUp();

    BrowserManager::Get()->AddObserver(this);
  }

  void TearDown() override {
    BrowserManager::Get()->RemoveObserver(this);

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

  // Assert
  const bool has_instance = BrowserManager::HasInstance();
  EXPECT_TRUE(has_instance);
}

TEST_F(BatAdsBrowserManagerTest, BrowserDidBecomeActive) {
  // Arrange
  BrowserManager::Get()->SetForeground(true);
  BrowserManager::Get()->SetActive(false);

  // Act
  BrowserManager::Get()->OnDidBecomeActive();

  // Assert
  EXPECT_TRUE(BrowserManager::Get()->IsActive());
  EXPECT_TRUE(browser_did_become_active_);
  EXPECT_FALSE(browser_did_resign_active_);
}

TEST_F(BatAdsBrowserManagerTest, BrowserDidResignActive) {
  // Arrange
  BrowserManager::Get()->SetActive(true);

  // Act
  BrowserManager::Get()->OnDidResignActive();

  // Assert
  EXPECT_FALSE(BrowserManager::Get()->IsActive());
  EXPECT_FALSE(browser_did_become_active_);
  EXPECT_TRUE(browser_did_resign_active_);
}

TEST_F(BatAdsBrowserManagerTest, BrowserDidEnterForeground) {
  // Arrange

  // Act
  BrowserManager::Get()->OnDidEnterForeground();

  // Assert
  EXPECT_TRUE(BrowserManager::Get()->IsForeground());
  EXPECT_TRUE(browser_did_enter_foreground_);
  EXPECT_FALSE(browser_did_enter_background_);
}

TEST_F(BatAdsBrowserManagerTest, BrowserDidEnterBackground) {
  // Arrange
  BrowserManager::Get()->SetForeground(true);

  // Act
  BrowserManager::Get()->OnDidEnterBackground();

  // Assert
  EXPECT_FALSE(BrowserManager::Get()->IsForeground());
  EXPECT_FALSE(browser_did_enter_foreground_);
  EXPECT_TRUE(browser_did_enter_background_);
}

}  // namespace ads
