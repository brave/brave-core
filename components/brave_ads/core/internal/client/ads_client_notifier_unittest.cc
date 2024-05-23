/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/client/ads_client_notifier.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_notifier_observer_mock.h"
#include "testing/gmock/include/gmock/gmock.h"  // IWYU pragma: keep
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr char kLocale[] = "Locale";
constexpr char kPrefPath[] = "PrefPath";
constexpr char kManifestVersion[] = "ManifestVersion";
constexpr char kResourceId[] = "ResourceId";
constexpr char kPaymentId[] = "PaymentId";
constexpr char kRecoverySeed[] = "RecoverySeed";
constexpr char kRedirectChainUrl[] = "https://example.com";
constexpr char kText[] = "Text content";
constexpr char kHtml[] = "HTML content";

constexpr int32_t kTabId = 1;
constexpr int32_t kPageTransitionType = 2;

constexpr bool kIsVisible = true;
constexpr bool kScreenWasLocked = true;

constexpr base::TimeDelta kIdleTime = base::Minutes(1);

void Notify(const AdsClientNotifier& queued_notifier) {
  queued_notifier.NotifyDidInitializeAds();
  queued_notifier.NotifyLocaleDidChange(kLocale);
  queued_notifier.NotifyPrefDidChange(kPrefPath);
  queued_notifier.NotifyDidUpdateResourceComponent(kManifestVersion,
                                                   kResourceId);
  queued_notifier.NotifyDidUnregisterResourceComponent(kResourceId);
  queued_notifier.NotifyRewardsWalletDidUpdate(kPaymentId, kRecoverySeed);
  queued_notifier.NotifyTabTextContentDidChange(
      kTabId, {GURL(kRedirectChainUrl)}, kText);
  queued_notifier.NotifyTabHtmlContentDidChange(
      kTabId, {GURL(kRedirectChainUrl)}, kHtml);
  queued_notifier.NotifyTabDidStartPlayingMedia(kTabId);
  queued_notifier.NotifyTabDidStopPlayingMedia(kTabId);
  queued_notifier.NotifyTabDidChange(
      kTabId, {GURL(kRedirectChainUrl)}, /*is_new_navigation=*/true,
      /*is_restoring=*/false, /*is_error_page=*/false, kIsVisible);
  queued_notifier.NotifyDidCloseTab(kTabId);
  queued_notifier.NotifyUserGestureEventTriggered(kPageTransitionType);
  queued_notifier.NotifyUserDidBecomeIdle();
  queued_notifier.NotifyUserDidBecomeActive(kIdleTime, kScreenWasLocked);
  queued_notifier.NotifyBrowserDidEnterForeground();
  queued_notifier.NotifyBrowserDidEnterBackground();
  queued_notifier.NotifyBrowserDidBecomeActive();
  queued_notifier.NotifyBrowserDidResignActive();
  queued_notifier.NotifyDidSolveAdaptiveCaptcha();
}

void ExpectNotifierCalls(AdsClientNotifierObserverMock& observer,
                         int expected_calls_count) {
  EXPECT_CALL(observer, OnNotifyDidInitializeAds()).Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyLocaleDidChange(kLocale))
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyPrefDidChange(kPrefPath))
      .Times(expected_calls_count);
  EXPECT_CALL(observer,
              OnNotifyDidUpdateResourceComponent(kManifestVersion, kResourceId))
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyDidUnregisterResourceComponent(kResourceId))
      .Times(expected_calls_count);
  EXPECT_CALL(observer,
              OnNotifyRewardsWalletDidUpdate(kPaymentId, kRecoverySeed))
      .Times(expected_calls_count);
  EXPECT_CALL(
      observer,
      OnNotifyTabTextContentDidChange(
          kTabId, ::testing::ElementsAre(GURL(kRedirectChainUrl)), kText))
      .Times(expected_calls_count);
  EXPECT_CALL(
      observer,
      OnNotifyTabHtmlContentDidChange(
          kTabId, ::testing::ElementsAre(GURL(kRedirectChainUrl)), kHtml))
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyTabDidStartPlayingMedia(kTabId))
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyTabDidStopPlayingMedia(kTabId))
      .Times(expected_calls_count);
  EXPECT_CALL(observer,
              OnNotifyTabDidChange(
                  kTabId, ::testing::ElementsAre(GURL(kRedirectChainUrl)),
                  /*is_new_navigation=*/true, /*is_restoring=*/false,
                  /*is_error_page=*/false, kIsVisible))
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyDidCloseTab(kTabId))
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyUserGestureEventTriggered(kPageTransitionType))
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyUserDidBecomeIdle())
      .Times(expected_calls_count);
  EXPECT_CALL(observer,
              OnNotifyUserDidBecomeActive(kIdleTime, kScreenWasLocked))
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyBrowserDidEnterForeground())
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyBrowserDidEnterBackground())
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyBrowserDidBecomeActive())
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyBrowserDidResignActive())
      .Times(expected_calls_count);
  EXPECT_CALL(observer, OnNotifyDidSolveAdaptiveCaptcha())
      .Times(expected_calls_count);
}

}  // namespace

TEST(BraveAdsAdsClientNotifierTest, FireQueuedNotifications) {
  // Arrange
  AdsClientNotifier queued_notifier;
  queued_notifier.set_should_queue_notifications_for_testing(
      /*should_queue_notifications=*/true);

  ::testing::StrictMock<AdsClientNotifierObserverMock> observer;
  queued_notifier.AddObserver(&observer);

  // Act & Assert
  ExpectNotifierCalls(observer, /*expected_calls_count=*/0);
  Notify(queued_notifier);
  EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(&observer));

  // Act & Assert
  ExpectNotifierCalls(observer, /*expected_calls_count=*/1);
  queued_notifier.NotifyPendingObservers();
  EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(&observer));

  // Act & Assert
  ExpectNotifierCalls(observer, /*expected_calls_count=*/1);
  Notify(queued_notifier);
  EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(&observer));

  queued_notifier.RemoveObserver(&observer);
}

TEST(BraveAdsAdsClientNotifierTest, NotificationsNotFiredIfWereQueued) {
  // Arrange
  ::testing::StrictMock<AdsClientNotifierObserverMock> observer;
  ExpectNotifierCalls(observer, /*expected_calls_count=*/0);

  AdsClientNotifier queued_notifier;
  queued_notifier.set_should_queue_notifications_for_testing(
      /*should_queue_notifications=*/true);

  queued_notifier.AddObserver(&observer);

  // Act & Assert
  Notify(queued_notifier);
  queued_notifier.RemoveObserver(&observer);
}

TEST(BraveAdsAdsClientNotifierTest, ShouldNotQueueNotifications) {
  // Arrange
  ::testing::StrictMock<AdsClientNotifierObserverMock> observer;
  ExpectNotifierCalls(observer, /*expected_calls_count=*/1);

  AdsClientNotifier queued_notifier;
  queued_notifier.set_should_queue_notifications_for_testing(
      /*should_queue_notifications=*/false);
  queued_notifier.AddObserver(&observer);

  // Act & Assert
  Notify(queued_notifier);
  queued_notifier.RemoveObserver(&observer);
}

}  // namespace brave_ads
