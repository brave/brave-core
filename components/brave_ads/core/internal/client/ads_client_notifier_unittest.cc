/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/client/ads_client_notifier.h"

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_notifier_observer_mock.h"
#include "testing/gmock/include/gmock/gmock.h"
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
constexpr char kRedirectChainUrl[] = "https://brave.com";
constexpr char kText[] = "Text";
constexpr char kHtml[] = "HTML";

constexpr int32_t kTabId = 1;
constexpr bool kIsNewNavigation = true;
constexpr bool kIsRestoring = false;
constexpr bool kIsErrorPage = false;
constexpr bool kIsVisible = true;

constexpr int32_t kPageTransitionType = 2;

constexpr base::TimeDelta kIdleTime = base::Minutes(1);
constexpr bool kScreenWasLocked = true;

}  // namespace

class BraveAdsAdsClientNotifierTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ads_client_notifier_.AddObserver(&ads_client_notifier_observer_mock_);
  }

  void TearDown() override {
    ads_client_notifier_.RemoveObserver(&ads_client_notifier_observer_mock_);
  }

  void FireAdsClientNotifiers() {
    ads_client_notifier_.NotifyDidInitializeAds();

    ads_client_notifier_.NotifyLocaleDidChange(kLocale);

    ads_client_notifier_.NotifyPrefDidChange(kPrefPath);

    ads_client_notifier_.NotifyResourceComponentDidChange(kManifestVersion,
                                                          kResourceId);
    ads_client_notifier_.NotifyDidUnregisterResourceComponent(kResourceId);

    ads_client_notifier_.NotifyRewardsWalletDidUpdate(kPaymentId,
                                                      kRecoverySeed);

    ads_client_notifier_.NotifyTabTextContentDidChange(
        kTabId, {GURL(kRedirectChainUrl)}, kText);
    ads_client_notifier_.NotifyTabHtmlContentDidChange(
        kTabId, {GURL(kRedirectChainUrl)}, kHtml);
    ads_client_notifier_.NotifyTabDidStartPlayingMedia(kTabId);
    ads_client_notifier_.NotifyTabDidStopPlayingMedia(kTabId);
    ads_client_notifier_.NotifyTabDidChange(kTabId, {GURL(kRedirectChainUrl)},
                                            kIsNewNavigation, kIsRestoring,
                                            kIsErrorPage, kIsVisible);
    ads_client_notifier_.NotifyDidCloseTab(kTabId);

    ads_client_notifier_.NotifyUserGestureEventTriggered(kPageTransitionType);
    ads_client_notifier_.NotifyUserDidBecomeIdle();
    ads_client_notifier_.NotifyUserDidBecomeActive(kIdleTime, kScreenWasLocked);

    ads_client_notifier_.NotifyBrowserDidEnterForeground();
    ads_client_notifier_.NotifyBrowserDidEnterBackground();
    ads_client_notifier_.NotifyBrowserDidBecomeActive();
    ads_client_notifier_.NotifyBrowserDidResignActive();

    ads_client_notifier_.NotifyDidSolveAdaptiveCaptcha();
  }

  void ExpectAdsClientNotifierCallCount(const int expected_call_count) {
    EXPECT_CALL(ads_client_notifier_observer_mock_, OnNotifyDidInitializeAds())
        .Times(expected_call_count);

    EXPECT_CALL(ads_client_notifier_observer_mock_,
                OnNotifyLocaleDidChange(kLocale))
        .Times(expected_call_count);

    EXPECT_CALL(ads_client_notifier_observer_mock_,
                OnNotifyPrefDidChange(kPrefPath))
        .Times(expected_call_count);

    EXPECT_CALL(
        ads_client_notifier_observer_mock_,
        OnNotifyResourceComponentDidChange(kManifestVersion, kResourceId))
        .Times(expected_call_count);
    EXPECT_CALL(ads_client_notifier_observer_mock_,
                OnNotifyDidUnregisterResourceComponent(kResourceId))
        .Times(expected_call_count);

    EXPECT_CALL(ads_client_notifier_observer_mock_,
                OnNotifyRewardsWalletDidUpdate(kPaymentId, kRecoverySeed))
        .Times(expected_call_count);

    EXPECT_CALL(
        ads_client_notifier_observer_mock_,
        OnNotifyTabTextContentDidChange(
            kTabId, ::testing::ElementsAre(GURL(kRedirectChainUrl)), kText))
        .Times(expected_call_count);
    EXPECT_CALL(
        ads_client_notifier_observer_mock_,
        OnNotifyTabHtmlContentDidChange(
            kTabId, ::testing::ElementsAre(GURL(kRedirectChainUrl)), kHtml))
        .Times(expected_call_count);
    EXPECT_CALL(ads_client_notifier_observer_mock_,
                OnNotifyTabDidStartPlayingMedia(kTabId))
        .Times(expected_call_count);
    EXPECT_CALL(ads_client_notifier_observer_mock_,
                OnNotifyTabDidStopPlayingMedia(kTabId))
        .Times(expected_call_count);
    EXPECT_CALL(ads_client_notifier_observer_mock_,
                OnNotifyTabDidChange(
                    kTabId, ::testing::ElementsAre(GURL(kRedirectChainUrl)),
                    kIsNewNavigation, kIsRestoring, kIsErrorPage, kIsVisible))
        .Times(expected_call_count);
    EXPECT_CALL(ads_client_notifier_observer_mock_, OnNotifyDidCloseTab(kTabId))
        .Times(expected_call_count);

    EXPECT_CALL(ads_client_notifier_observer_mock_,
                OnNotifyUserGestureEventTriggered(kPageTransitionType))
        .Times(expected_call_count);
    EXPECT_CALL(ads_client_notifier_observer_mock_, OnNotifyUserDidBecomeIdle())
        .Times(expected_call_count);
    EXPECT_CALL(ads_client_notifier_observer_mock_,
                OnNotifyUserDidBecomeActive(kIdleTime, kScreenWasLocked))
        .Times(expected_call_count);

    EXPECT_CALL(ads_client_notifier_observer_mock_,
                OnNotifyBrowserDidEnterForeground())
        .Times(expected_call_count);
    EXPECT_CALL(ads_client_notifier_observer_mock_,
                OnNotifyBrowserDidEnterBackground())
        .Times(expected_call_count);
    EXPECT_CALL(ads_client_notifier_observer_mock_,
                OnNotifyBrowserDidBecomeActive())
        .Times(expected_call_count);
    EXPECT_CALL(ads_client_notifier_observer_mock_,
                OnNotifyBrowserDidResignActive())
        .Times(expected_call_count);

    EXPECT_CALL(ads_client_notifier_observer_mock_,
                OnNotifyDidSolveAdaptiveCaptcha())
        .Times(expected_call_count);
  }

  AdsClientNotifier ads_client_notifier_;

  ::testing::StrictMock<AdsClientNotifierObserverMock>
      ads_client_notifier_observer_mock_;
};

TEST_F(BraveAdsAdsClientNotifierTest, FireQueuedAdsClientNotifications) {
  // Arrange
  ads_client_notifier_.set_should_queue_notifications_for_testing(
      /*should_queue_notifications=*/true);

  // Act & Assert
  ExpectAdsClientNotifierCallCount(0);
  FireAdsClientNotifiers();  // Queue notifications.

  EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(
      &ads_client_notifier_observer_mock_));
  ExpectAdsClientNotifierCallCount(1);  // Fire queued notifications.
  ads_client_notifier_.NotifyPendingObservers();

  EXPECT_TRUE(::testing::Mock::VerifyAndClearExpectations(
      &ads_client_notifier_observer_mock_));
  ExpectAdsClientNotifierCallCount(0);  // Already fired queued notifications.
  ads_client_notifier_.NotifyPendingObservers();
}

TEST_F(
    BraveAdsAdsClientNotifierTest,
    DoNotFireQueuedAdsClientNotificationsIfNotifyPendingObserversIsNotCalled) {
  // Arrange
  ads_client_notifier_.set_should_queue_notifications_for_testing(
      /*should_queue_notifications=*/true);

  // Act & Assert
  ExpectAdsClientNotifierCallCount(0);
  FireAdsClientNotifiers();
}

TEST_F(BraveAdsAdsClientNotifierTest,
       FireAdsClientNotificationsImmediatelyIfNotQueued) {
  // Arrange
  ads_client_notifier_.set_should_queue_notifications_for_testing(
      /*should_queue_notifications=*/false);

  // Act & Assert
  ExpectAdsClientNotifierCallCount(1);
  FireAdsClientNotifiers();
}

}  // namespace brave_ads
