// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_ads/ads_service_impl_ios.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/check.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/browser/service/test/ads_service_observer_mock.h"
#include "brave/components/brave_ads/core/browser/service/test/ads_service_waiter.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_info.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"
#include "brave/components/brave_ads/core/public/test/ads_client_mock.h"
#include "brave/components/brave_ads/core/public/test/ads_mock.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_rewards/core/pref_registry.h"
#include "brave/ios/browser/brave_ads/test/fake_ads_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"
#include "url/gurl.h"

namespace brave_ads {

class BraveAdsServiceImplIOSTest : public PlatformTest {
 public:
  BraveAdsServiceImplIOSTest() {
    CHECK(temp_dir_.CreateUniqueTempDir());

    RegisterProfilePrefs(prefs_.registry());
    brave_rewards::RegisterProfilePrefs(prefs_.registry());

    auto ads_factory = std::make_unique<test::FakeAdsFactory>();
    ads_factory_ = ads_factory.get();
    ads_service_ =
        std::make_unique<AdsServiceImplIOS>(prefs_, std::move(ads_factory));
  }

 protected:
  // Blocks until every task already posted to the current sequence has run,
  // including tasks posted by production code from a pref change
  // notification (which cannot run synchronously; see
  // `AdsServiceImplIOS::OnAdsPrefChanged`).
  void FlushPendingTasks() {
    bool did_run = false;
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindLambdaForTesting([&] { did_run = true; }));
    ASSERT_TRUE(base::test::RunUntil([&] { return did_run; }));
  }

  bool InitializeAdsSuccessfully() {
    base::test::TestFuture<bool> test_future;
    ads_service_->InitializeAds(
        storage_path(),
        std::make_unique<::testing::NiceMock<test::AdsClientMock>>(),
        mojom::SysInfo::New(), mojom::BuildChannelInfo::New(),
        mojom::WalletInfo::New(), test_future.GetCallback());
    return test_future.Get();
  }

  std::string storage_path() const {
    return temp_dir_.GetPath().AsUTF8Unsafe();
  }

  test::AdsMock& GetAds() {
    test::AdsMock* ads = ads_factory_->GetAds();
    CHECK(ads);
    return *ads;
  }

  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<AdsServiceImplIOS> ads_service_;
  raw_ptr<test::FakeAdsFactory> ads_factory_;
};

TEST_F(BraveAdsServiceImplIOSTest,
       ClearsAdsDataWhenSponsoredAdsBecomeDisabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  // `ClearAdsPrefs` clears the whole `brave.brave_ads.*` prefix at once, so
  // checking this one pref is enough to tell whether the whole prefix was
  // cleared.
  prefs_.SetString(prefs::kDiagnosticId, "foo");
  test::AdsServiceWaiter waiter(*ads_service_);

  // Act
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  waiter.WaitForOnDidClearAdsServiceData();

  // Assert
  EXPECT_FALSE(prefs_.HasPrefPath(prefs::kDiagnosticId));
  EXPECT_FALSE(prefs_.GetBoolean(prefs::kSponsoredEnabled));
}

TEST_F(BraveAdsServiceImplIOSTest,
       DoesNotClearAdsDataWhenUnrelatedPrefChanges) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  prefs_.SetString(prefs::kDiagnosticId, "foo");

  // Act
  prefs_.SetBoolean(prefs::kNotificationsEnabled, true);

  // Assert
  EXPECT_EQ("foo", prefs_.GetString(prefs::kDiagnosticId));
}

TEST_F(BraveAdsServiceImplIOSTest,
       DoesNotClearAdsDataWhenSponsoredAdsAreEnabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  prefs_.SetString(prefs::kDiagnosticId, "foo");

  // Act
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);

  // Assert
  EXPECT_EQ("foo", prefs_.GetString(prefs::kDiagnosticId));
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearsAdsDataOnEachSuccessiveSponsoredAdsDisable) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  prefs_.SetString(prefs::kDiagnosticId, "foo");
  test::AdsServiceWaiter waiter1(*ads_service_);

  // Act
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  waiter1.WaitForOnDidClearAdsServiceData();

  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  prefs_.SetString(prefs::kDiagnosticId, "bar");
  test::AdsServiceWaiter waiter2(*ads_service_);
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  waiter2.WaitForOnDidClearAdsServiceData();

  // Assert
  EXPECT_FALSE(prefs_.HasPrefPath(prefs::kDiagnosticId));
  EXPECT_FALSE(prefs_.GetBoolean(prefs::kSponsoredEnabled));
}

TEST_F(BraveAdsServiceImplIOSTest,
       KeyedServiceShutdownNotifiesObserversWhenNotInitialized) {
  // Arrange
  test::AdsServiceWaiter waiter(*ads_service_);

  // Act
  static_cast<KeyedService&>(*ads_service_).Shutdown();

  // Assert
  waiter.WaitForOnDidShutdownAdsService();
}

TEST_F(BraveAdsServiceImplIOSTest,
       KeyedServiceShutdownNotifiesObserversWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  test::AdsServiceWaiter waiter(*ads_service_);

  // Act
  static_cast<KeyedService&>(*ads_service_).Shutdown();

  // Assert
  waiter.WaitForOnDidShutdownAdsService();
  EXPECT_FALSE(ads_service_->IsInitialized());
}

TEST_F(BraveAdsServiceImplIOSTest,
       IsNotInitializedBeforeInitializeAdsIsCalled) {
  // Act & Assert
  EXPECT_FALSE(ads_service_->IsInitialized());
}

TEST_F(BraveAdsServiceImplIOSTest,
       DoesNotInitializeAdsWhenRewardsIsDisabledByPolicy) {
  // Arrange
  prefs_.SetManagedPref(brave_rewards::prefs::kDisabledByPolicy,
                        base::Value(true));
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->InitializeAds(
      storage_path(), /*ads_client=*/nullptr, mojom::SysInfo::New(),
      mojom::BuildChannelInfo::New(), mojom::WalletInfo::New(),
      test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
  EXPECT_FALSE(ads_service_->IsInitialized());
}

TEST_F(BraveAdsServiceImplIOSTest,
       InitializeAdsSucceedsAndNotifiesObserversWhenAllowedToStart) {
  // Arrange
  ::testing::NiceMock<test::AdsServiceObserverMock> observer;
  base::ScopedObservation<AdsService, AdsServiceObserver> observation(
      &observer);
  observation.Observe(ads_service_.get());

  // Act & Assert
  EXPECT_CALL(observer, OnDidInitializeAdsService);
  EXPECT_TRUE(InitializeAdsSuccessfully());
  EXPECT_TRUE(ads_service_->IsInitialized());
}

TEST_F(BraveAdsServiceImplIOSTest, InitializeAdsFailsWhenAlreadyInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  ASSERT_EQ(1U, ads_factory_->created_ads_count());
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->InitializeAds(
      storage_path(),
      std::make_unique<::testing::NiceMock<test::AdsClientMock>>(),
      mojom::SysInfo::New(), mojom::BuildChannelInfo::New(),
      mojom::WalletInfo::New(), test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
  EXPECT_EQ(1U, ads_factory_->created_ads_count());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ShutdownAdsSucceedsImmediatelyWhenNotRunning) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->ShutdownAds(test_future.GetCallback());

  // Assert
  EXPECT_TRUE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ShutdownAdsSucceedsAndNotifiesObserversWhenRunning) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  ::testing::NiceMock<test::AdsServiceObserverMock> observer;
  base::ScopedObservation<AdsService, AdsServiceObserver> observation(
      &observer);
  observation.Observe(ads_service_.get());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(observer, OnDidShutdownAdsService);
  ads_service_->ShutdownAds(test_future.GetCallback());
  EXPECT_TRUE(test_future.Get());
  EXPECT_FALSE(ads_service_->IsInitialized());
}

TEST_F(BraveAdsServiceImplIOSTest,
       MaybeGetNotificationAdReturnsNulloptWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<base::optional_ref<const NotificationAdInfo>>
      test_future;

  // Act
  ads_service_->MaybeGetNotificationAd("placement_id",
                                       test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       MaybeGetNotificationAdForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<base::optional_ref<const NotificationAdInfo>>
      test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), MaybeGetNotificationAd("placement_id", ::testing::_));
  ads_service_->MaybeGetNotificationAd("placement_id",
                                       test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       TriggerNotificationAdEventFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->TriggerNotificationAdEvent(
      "placement_id", mojom::NotificationAdEventType::kClicked,
      test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       TriggerNotificationAdEventForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(),
              TriggerNotificationAdEvent(
                  "placement_id", mojom::NotificationAdEventType::kClicked,
                  ::testing::_));
  ads_service_->TriggerNotificationAdEvent(
      "placement_id", mojom::NotificationAdEventType::kClicked,
      test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       GetInternalsReturnsNulloptWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<std::optional<base::DictValue>> test_future;

  // Act
  ads_service_->GetInternals(test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest, GetInternalsForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<std::optional<base::DictValue>> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), GetInternals);
  ads_service_->GetInternals(test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       GetDiagnosticsReturnsNulloptWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<std::optional<base::DictValue>> test_future;

  // Act
  ads_service_->GetDiagnostics(test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest, GetDiagnosticsForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<std::optional<base::DictValue>> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), GetDiagnostics);
  ads_service_->GetDiagnostics(test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       EvaluateConditionMatcherReturnsUnknownWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<const std::string&, const std::string&> test_future;

  // Act
  ads_service_->EvaluateConditionMatcher("pref_path", "condition",
                                         /*test_value=*/std::nullopt,
                                         test_future.GetCallback());

  // Assert
  auto& [current_value, matches] = test_future.Get();
  EXPECT_EQ("Unknown", current_value);
  EXPECT_EQ("N/A", matches);
}

TEST_F(BraveAdsServiceImplIOSTest,
       EvaluateConditionMatcherForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<const std::string&, const std::string&> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(),
              EvaluateConditionMatcher(
                  "pref_path", "condition",
                  /*test_value=*/std::optional<std::string>(std::nullopt),
                  ::testing::_));
  ads_service_->EvaluateConditionMatcher("pref_path", "condition",
                                         /*test_value=*/std::nullopt,
                                         test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       GetStatementOfAccountsReturnsNullWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<mojom::StatementInfoPtr> test_future;

  // Act
  ads_service_->GetStatementOfAccounts(test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       GetStatementOfAccountsForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<mojom::StatementInfoPtr> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), GetStatementOfAccounts);
  ads_service_->GetStatementOfAccounts(test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ParseAndSaveNewTabPageAdsQueuesUntilServiceInitializes) {
  // Arrange
  base::test::TestFuture<bool> test_future;
  ads_service_->ParseAndSaveNewTabPageAds(base::DictValue(),
                                          test_future.GetCallback());
  ASSERT_FALSE(test_future.IsReady());

  // Act
  ASSERT_TRUE(InitializeAdsSuccessfully());

  // Assert
  EXPECT_TRUE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ParseAndSaveNewTabPageAdsForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), ParseAndSaveNewTabPageAds);
  ads_service_->ParseAndSaveNewTabPageAds(base::DictValue(),
                                          test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       MaybeServeNewTabPageAdReturnsNullWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<mojom::NewTabPageAdInfoPtr> test_future;

  // Act
  ads_service_->MaybeServeNewTabPageAd(test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       MaybeServeNewTabPageAdForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<mojom::NewTabPageAdInfoPtr> test_future;

  NewTabPageAdInfo ad;
  ad.type = mojom::AdType::kNewTabPageAd;
  ad.placement_id = "placement_id";
  ad.creative_instance_id = "creative_instance_id";
  ad.creative_set_id = "creative_set_id";
  ad.campaign_id = "campaign_id";
  ad.advertiser_id = "advertiser_id";
  ad.segment = "segment";
  ad.target_url = GURL("https://brave.com");
  ad.company_name = "company_name";
  ad.alt = "alt";

  // Act & Assert
  EXPECT_CALL(GetAds(), MaybeServeNewTabPageAd)
      .WillOnce(base::test::RunOnceCallback<0>(ad));
  ads_service_->MaybeServeNewTabPageAd(test_future.GetCallback());
  const mojom::NewTabPageAdInfoPtr& served_ad = test_future.Get();
  ASSERT_TRUE(served_ad);
  EXPECT_EQ(ad.placement_id, served_ad->placement_id);
  EXPECT_EQ(ad.creative_instance_id, served_ad->creative_instance_id);
}

TEST_F(BraveAdsServiceImplIOSTest,
       TriggerNewTabPageAdEventFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->TriggerNewTabPageAdEvent(
      "placement_id", "creative_instance_id",
      mojom::NewTabPageAdMetricType::kConfirmation,
      mojom::NewTabPageAdEventType::kClicked, test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       TriggerNewTabPageAdEventForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(),
              TriggerNewTabPageAdEvent(
                  "placement_id", "creative_instance_id",
                  mojom::NewTabPageAdMetricType::kConfirmation,
                  mojom::NewTabPageAdEventType::kClicked, ::testing::_));
  ads_service_->TriggerNewTabPageAdEvent(
      "placement_id", "creative_instance_id",
      mojom::NewTabPageAdMetricType::kConfirmation,
      mojom::NewTabPageAdEventType::kClicked, test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       MaybeGetSearchResultAdReturnsNullWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<mojom::CreativeSearchResultAdInfoPtr> test_future;

  // Act
  ads_service_->MaybeGetSearchResultAd("placement_id",
                                       test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       MaybeGetSearchResultAdForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<mojom::CreativeSearchResultAdInfoPtr> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), MaybeGetSearchResultAd("placement_id", ::testing::_));
  ads_service_->MaybeGetSearchResultAd("placement_id",
                                       test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       TriggerSearchResultAdEventFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->TriggerSearchResultAdEvent(
      mojom::CreativeSearchResultAdInfo::New(),
      mojom::SearchResultAdEventType::kClicked, test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       TriggerSearchResultAdEventForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), TriggerSearchResultAdEvent);
  ads_service_->TriggerSearchResultAdEvent(
      mojom::CreativeSearchResultAdInfo::New(),
      mojom::SearchResultAdEventType::kClicked, test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       PurgeOrphanedAdEventsForTypeFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->PurgeOrphanedAdEventsForType(mojom::AdType::kNewTabPageAd,
                                             test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       PurgeOrphanedAdEventsForTypeForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), PurgeOrphanedAdEventsForType(
                            mojom::AdType::kNewTabPageAd, ::testing::_));
  ads_service_->PurgeOrphanedAdEventsForType(mojom::AdType::kNewTabPageAd,
                                             test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       GetAdHistoryReturnsNulloptWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<
      std::optional<std::vector<mojom::AdHistoryItemInfoPtr>>>
      test_future;

  // Act
  ads_service_->GetAdHistory(base::Time(), base::Time::Now(),
                             test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest, GetAdHistoryForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<
      std::optional<std::vector<mojom::AdHistoryItemInfoPtr>>>
      test_future;
  const base::Time from_time;
  const base::Time to_time = base::Time::Now();

  // Act & Assert
  EXPECT_CALL(GetAds(), GetAdHistory(from_time, to_time, ::testing::_));
  ads_service_->GetAdHistory(from_time, to_time, test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest, ToggleLikeAdFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->ToggleLikeAd(mojom::ReactionInfo::New(),
                             test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest, ToggleLikeAdForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), ToggleLikeAd);
  ads_service_->ToggleLikeAd(mojom::ReactionInfo::New(),
                             test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest, ToggleDislikeAdFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->ToggleDislikeAd(mojom::ReactionInfo::New(),
                                test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ToggleDislikeAdForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), ToggleDislikeAd);
  ads_service_->ToggleDislikeAd(mojom::ReactionInfo::New(),
                                test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest, ToggleLikeSegmentFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->ToggleLikeSegment(mojom::ReactionInfo::New(),
                                  test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ToggleLikeSegmentForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), ToggleLikeSegment);
  ads_service_->ToggleLikeSegment(mojom::ReactionInfo::New(),
                                  test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ToggleDislikeSegmentFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->ToggleDislikeSegment(mojom::ReactionInfo::New(),
                                     test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ToggleDislikeSegmentForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), ToggleDislikeSegment);
  ads_service_->ToggleDislikeSegment(mojom::ReactionInfo::New(),
                                     test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest, ToggleSaveAdFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->ToggleSaveAd(mojom::ReactionInfo::New(),
                             test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest, ToggleSaveAdForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), ToggleSaveAd);
  ads_service_->ToggleSaveAd(mojom::ReactionInfo::New(),
                             test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ToggleMarkAdAsInappropriateFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->ToggleMarkAdAsInappropriate(mojom::ReactionInfo::New(),
                                            test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ToggleMarkAdAsInappropriateForwardsToAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), ToggleMarkAdAsInappropriate);
  ads_service_->ToggleMarkAdAsInappropriate(mojom::ReactionInfo::New(),
                                            test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       NotifyDidInitializeAdsServiceNotifiesObservers) {
  // Arrange
  ::testing::NiceMock<test::AdsServiceObserverMock> observer;
  base::ScopedObservation<AdsService, AdsServiceObserver> observation(
      &observer);
  observation.Observe(ads_service_.get());

  // Act & Assert
  EXPECT_CALL(observer, OnDidInitializeAdsService);
  ads_service_->NotifyDidInitializeAdsService();
}

TEST_F(BraveAdsServiceImplIOSTest,
       NotifyDidShutdownAdsServiceNotifiesObservers) {
  // Arrange
  ::testing::NiceMock<test::AdsServiceObserverMock> observer;
  base::ScopedObservation<AdsService, AdsServiceObserver> observation(
      &observer);
  observation.Observe(ads_service_.get());

  // Act & Assert
  EXPECT_CALL(observer, OnDidShutdownAdsService);
  ads_service_->NotifyDidShutdownAdsService();
}

TEST_F(BraveAdsServiceImplIOSTest, ClearDataClearsAdsPrefs) {
  // Arrange
  prefs_.SetString(prefs::kDiagnosticId, "foo");
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->ClearData(test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_FALSE(prefs_.HasPrefPath(prefs::kDiagnosticId));
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearDataPreservesSponsoredEnabledPrefWhenSponsoredAdsAreDisabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->ClearData(test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_FALSE(prefs_.GetBoolean(prefs::kSponsoredEnabled));
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearDataPreservesSponsoredEnabledPrefWhenSponsoredAdsAreEnabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->ClearData(test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_TRUE(prefs_.GetBoolean(prefs::kSponsoredEnabled));
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearDataDoesNotSetSponsoredEnabledPrefWhenUnset) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->ClearData(test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_FALSE(prefs_.HasPrefPath(prefs::kSponsoredEnabled));
}

TEST_F(BraveAdsServiceImplIOSTest, ClearDataNotifiesObserversWhenNotRunning) {
  // Arrange
  test::AdsServiceWaiter waiter(*ads_service_);
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->ClearData(test_future.GetCallback());
  waiter.WaitForOnDidClearAdsServiceData();

  // Assert
  EXPECT_TRUE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearDataRestartsServiceWhenItWasRunningBeforeClear) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  ASSERT_EQ(1U, ads_factory_->created_ads_count());
  base::WeakPtr<test::AdsMock> previous_ads_instance = GetAds().GetWeakPtr();
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->ClearData(test_future.GetCallback());

  // Assert
  EXPECT_TRUE(test_future.Get());
  EXPECT_FALSE(previous_ads_instance);
  EXPECT_TRUE(ads_service_->IsInitialized());
  EXPECT_EQ(2U, ads_factory_->created_ads_count());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearDataSkipsRestartWhenAlreadyReinitializedDuringClear) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  ASSERT_EQ(1U, ads_factory_->created_ads_count());
  base::test::TestFuture<bool> clear_test_future;

  // Act
  ads_service_->ClearData(clear_test_future.GetCallback());
  base::test::TestFuture<bool> reinit_test_future;
  ads_service_->InitializeAds(
      storage_path(),
      std::make_unique<::testing::NiceMock<test::AdsClientMock>>(),
      mojom::SysInfo::New(), mojom::BuildChannelInfo::New(),
      mojom::WalletInfo::New(), reinit_test_future.GetCallback());
  ASSERT_TRUE(reinit_test_future.Get());
  ASSERT_EQ(2U, ads_factory_->created_ads_count());

  // Assert
  EXPECT_TRUE(clear_test_future.Get());
  EXPECT_TRUE(ads_service_->IsInitialized());
  EXPECT_EQ(2U, ads_factory_->created_ads_count());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearDataFailsAndPreservesPrefsWhenUnderlyingShutdownFails) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  ON_CALL(GetAds(), Shutdown)
      .WillByDefault(
          base::test::RunOnceCallbackRepeatedly<0>(/*success=*/false));
  prefs_.SetString(prefs::kDiagnosticId, "foo");
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service_->ClearData(test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
  EXPECT_EQ("foo", prefs_.GetString(prefs::kDiagnosticId));
}

TEST_F(BraveAdsServiceImplIOSTest,
       PreservesAdsDataWhenSponsoredAdsBecomeDisabledForBraveRewardsUser) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
  prefs_.SetString(prefs::kDiagnosticId, "foo");

  // Act
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  // Ensures `MaybeClearAdsData`'s posted task has had a
  // chance to (not) clear data before asserting.
  FlushPendingTasks();

  // Assert
  EXPECT_EQ("foo", prefs_.GetString(prefs::kDiagnosticId));
}

TEST_F(
    BraveAdsServiceImplIOSTest,
    PreservesAdsDataWhenBraveRewardsBecomesDisabledWhileSponsoredAdsRemainEnabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
  prefs_.SetString(prefs::kDiagnosticId, "foo");

  // Act
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, false);
  // Ensures `MaybeClearAdsData`'s posted task has had a
  // chance to (not) clear data before asserting.
  FlushPendingTasks();

  // Assert
  EXPECT_EQ("foo", prefs_.GetString(prefs::kDiagnosticId));
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearsAdsDataWhenBraveRewardsBecomesDisabledAndSponsoredAdsAreDisabled) {
  // Arrange
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  // `ClearAdsPrefs` clears the whole `brave.brave_ads.*` prefix at once, so
  // checking this one pref is enough to tell whether the whole prefix was
  // cleared.
  prefs_.SetString(prefs::kDiagnosticId, "foo");
  test::AdsServiceWaiter waiter(*ads_service_);

  // Act
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, false);
  waiter.WaitForOnDidClearAdsServiceData();

  // Assert
  EXPECT_FALSE(prefs_.HasPrefPath(prefs::kDiagnosticId));
}

TEST_F(BraveAdsServiceImplIOSTest,
       EligibleToStartWhenOnlySponsoredAdsAreEnabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);

  // Act
  ads_service_->InitializeAds(storage_path(), /*ads_client=*/nullptr,
                              mojom::SysInfo::New(),
                              mojom::BuildChannelInfo::New(),
                              /*mojom_wallet=*/nullptr, base::DoNothing());

  // Assert
  EXPECT_FALSE(ads_service_->IsIneligibleToStart());
}

TEST_F(BraveAdsServiceImplIOSTest,
       EligibleToStartWhenNotificationAdsAreEnabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
  prefs_.SetBoolean(prefs::kNotificationsEnabled, true);

  // Act
  ads_service_->InitializeAds(storage_path(), /*ads_client=*/nullptr,
                              mojom::SysInfo::New(),
                              mojom::BuildChannelInfo::New(),
                              /*mojom_wallet=*/nullptr, base::DoNothing());

  // Assert
  EXPECT_FALSE(ads_service_->IsIneligibleToStart());
}

TEST_F(BraveAdsServiceImplIOSTest, IneligibleToStartWhenAllAdTypesAreDisabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  test::AdsServiceWaiter waiter(*ads_service_);

  // Act
  bool success = true;
  ads_service_->InitializeAds(
      storage_path(), /*ads_client=*/nullptr, mojom::SysInfo::New(),
      mojom::BuildChannelInfo::New(), /*mojom_wallet=*/nullptr,
      base::BindLambdaForTesting(
          [&success](bool result) { success = result; }));

  // Assert
  waiter.WaitForOnAdsServiceIneligibleToStart();
  EXPECT_FALSE(success);
  EXPECT_TRUE(ads_service_->IsIneligibleToStart());
}

TEST_F(BraveAdsServiceImplIOSTest,
       BecomesIneligibleToStartWhenSponsoredAdsAreDisabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  test::AdsServiceWaiter waiter(*ads_service_);

  // Act
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);

  // Assert
  waiter.WaitForOnAdsServiceIneligibleToStart();
  EXPECT_TRUE(ads_service_->IsIneligibleToStart());
}

TEST_F(BraveAdsServiceImplIOSTest,
       BecomesIneligibleToStartWhenNotificationAdsAreDisabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
  prefs_.SetBoolean(prefs::kNotificationsEnabled, true);
  test::AdsServiceWaiter waiter(*ads_service_);

  // Act
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, false);
  prefs_.SetBoolean(prefs::kNotificationsEnabled, false);

  // Assert
  waiter.WaitForOnAdsServiceIneligibleToStart();
  EXPECT_TRUE(ads_service_->IsIneligibleToStart());
}

TEST_F(BraveAdsServiceImplIOSTest,
       IneligibleToStartWhenRewardsIsDisabledByPolicy) {
  // Arrange
  prefs_.SetManagedPref(brave_rewards::prefs::kDisabledByPolicy,
                        base::Value(true));
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  prefs_.SetBoolean(prefs::kNotificationsEnabled, true);
  test::AdsServiceWaiter waiter(*ads_service_);

  // Act
  bool success = true;
  ads_service_->InitializeAds(
      storage_path(), /*ads_client=*/nullptr, mojom::SysInfo::New(),
      mojom::BuildChannelInfo::New(), /*mojom_wallet=*/nullptr,
      base::BindLambdaForTesting(
          [&success](bool result) { success = result; }));

  // Assert
  waiter.WaitForOnAdsServiceIneligibleToStart();
  EXPECT_FALSE(success);
  EXPECT_TRUE(ads_service_->IsIneligibleToStart());
}

TEST_F(BraveAdsServiceImplIOSTest,
       OnAdsPrefChangedBecomesEligibleAgainReinitializesService) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  test::AdsServiceWaiter ineligible_waiter(*ads_service_);
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  ineligible_waiter.WaitForOnAdsServiceIneligibleToStart();
  ASSERT_FALSE(ads_service_->IsInitialized());
  test::AdsServiceWaiter reinitialize_waiter(*ads_service_);

  // Act
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  reinitialize_waiter.WaitForOnDidInitializeAdsService();

  // Assert
  EXPECT_FALSE(ads_service_->IsIneligibleToStart());
  EXPECT_TRUE(ads_service_->IsInitialized());
}

TEST_F(BraveAdsServiceImplIOSTest,
       OnAdsPrefChangedShutsDownRunningServiceWhenBecomingIneligible) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  test::AdsServiceWaiter waiter(*ads_service_);

  // Act
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);

  // Assert
  waiter.WaitForOnDidShutdownAdsService();
  EXPECT_FALSE(ads_service_->IsInitialized());
}

TEST_F(BraveAdsServiceImplIOSTest,
       DoesNotInitializeServiceWhenNotificationsEnabledPrefChangesAlone) {
  // Arrange
  ::testing::NiceMock<test::AdsServiceObserverMock> observer;
  base::ScopedObservation<AdsService, AdsServiceObserver> observation(
      &observer);
  observation.Observe(ads_service_.get());

  // Act & Assert
  EXPECT_CALL(observer, OnAdsServiceIneligibleToStart).Times(0);
  prefs_.SetBoolean(prefs::kNotificationsEnabled, true);
  EXPECT_FALSE(ads_service_->IsInitialized());
}

TEST_F(BraveAdsServiceImplIOSTest,
       NotifyAdsServiceIneligibleToStartOnlyNotifiesObserversOnce) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  ::testing::NiceMock<test::AdsServiceObserverMock> observer;
  base::ScopedObservation<AdsService, AdsServiceObserver> observation(
      &observer);
  observation.Observe(ads_service_.get());
  test::AdsServiceWaiter waiter(*ads_service_);
  ads_service_->InitializeAds(storage_path(), /*ads_client=*/nullptr,
                              mojom::SysInfo::New(),
                              mojom::BuildChannelInfo::New(),
                              /*mojom_wallet=*/nullptr, base::DoNothing());
  waiter.WaitForOnAdsServiceIneligibleToStart();
  ASSERT_TRUE(ads_service_->IsIneligibleToStart());

  // Act & Assert
  EXPECT_CALL(observer, OnAdsServiceIneligibleToStart).Times(0);
  prefs_.SetBoolean(prefs::kNotificationsEnabled, true);
  prefs_.SetBoolean(prefs::kNotificationsEnabled, false);
}

TEST_F(BraveAdsServiceImplIOSTest,
       NotifyDidInitializeAdsServiceResetsIneligibleFlag) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  test::AdsServiceWaiter ineligible_waiter(*ads_service_);
  ads_service_->InitializeAds(storage_path(), /*ads_client=*/nullptr,
                              mojom::SysInfo::New(),
                              mojom::BuildChannelInfo::New(),
                              /*mojom_wallet=*/nullptr, base::DoNothing());
  ineligible_waiter.WaitForOnAdsServiceIneligibleToStart();
  ASSERT_TRUE(ads_service_->IsIneligibleToStart());
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  ASSERT_TRUE(ads_service_->IsIneligibleToStart());

  // Act
  ASSERT_TRUE(InitializeAdsSuccessfully());

  // Assert
  EXPECT_FALSE(ads_service_->IsIneligibleToStart());
}

}  // namespace brave_ads
