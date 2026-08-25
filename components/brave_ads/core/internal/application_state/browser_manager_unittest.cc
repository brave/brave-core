/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/application_state/browser_manager.h"

#include "base/scoped_observation.h"
#include "brave/components/brave_ads/core/internal/application_state/browser_manager_observer.h"
#include "brave/components/brave_ads/core/internal/application_state/test/browser_manager_observer_mock.h"
#include "brave/components/brave_ads/core/internal/common/test/mock_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsBrowserManagerTest : public test::TestBase {
 protected:
  void SetUp() override {
    test::TestBase::SetUp();

    observation_.Observe(&BrowserManager::GetInstance());
  }

  BrowserManagerObserverMock browser_manager_observer_mock_;
  base::ScopedObservation<BrowserManager, BrowserManagerObserver> observation_{
      &browser_manager_observer_mock_};
};

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidBecomeActive) {
  // Arrange
  EXPECT_CALL(browser_manager_observer_mock_, OnBrowserDidBecomeActive);

  // Act
  ads_client_notifier_.NotifyBrowserDidBecomeActive();

  // Assert
  EXPECT_TRUE(BrowserManager::GetInstance().IsActive());
}

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidResignActive) {
  // Arrange
  ads_client_notifier_.NotifyBrowserDidBecomeActive();

  EXPECT_CALL(browser_manager_observer_mock_, OnBrowserDidResignActive);

  // Act
  ads_client_notifier_.NotifyBrowserDidResignActive();

  // Assert
  EXPECT_FALSE(BrowserManager::GetInstance().IsActive());
}

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidEnterForeground) {
  // Arrange
  EXPECT_CALL(browser_manager_observer_mock_, OnBrowserDidEnterForeground);

  // Act
  ads_client_notifier_.NotifyBrowserDidEnterForeground();

  // Assert
  EXPECT_TRUE(BrowserManager::GetInstance().IsInForeground());
}

TEST_F(BraveAdsBrowserManagerTest, OnNotifyBrowserDidEnterBackground) {
  // Arrange
  ads_client_notifier_.NotifyBrowserDidEnterForeground();

  EXPECT_CALL(browser_manager_observer_mock_, OnBrowserDidEnterBackground);

  // Act
  ads_client_notifier_.NotifyBrowserDidEnterBackground();

  // Assert
  EXPECT_FALSE(BrowserManager::GetInstance().IsInForeground());
}

TEST_F(BraveAdsBrowserManagerTest,
       OnNotifyDidInitializeAdsWhenBrowserIsActive) {
  // Act
  ads_client_notifier_.NotifyDidInitializeAds();

  // Assert
  EXPECT_TRUE(BrowserManager::GetInstance().IsActive());
  EXPECT_TRUE(BrowserManager::GetInstance().IsInForeground());
}

TEST_F(BraveAdsBrowserManagerTest,
       OnNotifyDidInitializeAdsWhenBrowserIsInactive) {
  // Arrange
  test::MockIsBrowserActive(ads_client_mock_, false);

  // Act
  ads_client_notifier_.NotifyDidInitializeAds();

  // Assert
  EXPECT_FALSE(BrowserManager::GetInstance().IsActive());
  EXPECT_FALSE(BrowserManager::GetInstance().IsInForeground());
}

// A racy `IsBrowserActive()` read at startup (see
// `InitializeBrowserBackgroundState()`) can report `false` for a browser
// window that is, in practice, already active; the native "did become
// active" notification that follows shortly after should also correct the
// stale foreground reading, since there is no separate foreground
// notification to rely on for that self-correction on desktop.
TEST_F(BraveAdsBrowserManagerTest,
       OnNotifyBrowserDidBecomeActiveCorrectsStaleForegroundState) {
  // Arrange
  test::MockIsBrowserActive(ads_client_mock_, false);
  ads_client_notifier_.NotifyDidInitializeAds();
  ASSERT_FALSE(BrowserManager::GetInstance().IsInForeground());

  // Act
  ads_client_notifier_.NotifyBrowserDidBecomeActive();

  // Assert
  EXPECT_TRUE(BrowserManager::GetInstance().IsInForeground());
}

// Losing window focus (e.g. alt-tabbing to another app) is not the same as
// the browser being minimized or occluded; the window can remain fully
// visible while inactive, so resigning active must not also flip foreground
// state to background.
TEST_F(BraveAdsBrowserManagerTest,
       OnNotifyBrowserDidResignActiveDoesNotAffectForegroundState) {
  // Arrange
  ads_client_notifier_.NotifyBrowserDidBecomeActive();
  ASSERT_TRUE(BrowserManager::GetInstance().IsInForeground());

  // Act
  ads_client_notifier_.NotifyBrowserDidResignActive();

  // Assert
  EXPECT_TRUE(BrowserManager::GetInstance().IsInForeground());
}

}  // namespace brave_ads
