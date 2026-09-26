// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_ads/ads_service_impl_ios.h"

#include "base/check.h"
#include "base/files/scoped_temp_dir.h"
#include "base/memory/raw_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/bind.h"
#include "base/test/gmock_callback_support.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/browser/service/test/ads_service_observer_mock.h"
#include "brave/components/brave_ads/core/browser/service/test/ads_service_waiter.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/notification_ad/notification_ad_info.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"
#include "brave/components/brave_ads/core/public/test/ads_mock.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/brave_rewards/core/pref_registry.h"
#include "brave/ios/browser/brave_ads/test/fake_ads_client.h"
#include "brave/ios/browser/brave_ads/test/fake_ads_factory.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace brave_ads {

using ::testing::NiceMock;
using ::testing::StrictMock;

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
        storage_path(), std::make_unique<test::FakeAdsClient>(),
        mojom::SysInfo::New(), mojom::BuildChannelInfo::New(),
        mojom::WalletInfo::New(), test_future.GetCallback());
    return test_future.Get();
  }

  std::string storage_path() const {
    return temp_dir_.GetPath().AsUTF8Unsafe();
  }

  AdsMock& GetAds() {
    AdsMock* ads = ads_factory_->GetAds();
    CHECK(ads);
    return *ads;
  }

  TestingPrefServiceSimple& prefs() { return prefs_; }

  test::FakeAdsFactory& ads_factory() { return *ads_factory_; }

  std::unique_ptr<AdsServiceImplIOS>& ads_service() { return ads_service_; }

  // Destroys `ads_service_`, clearing `ads_factory_` first so it does not
  // dangle once the `FakeAdsFactory` it owns is freed.
  void DestroyAdsService() {
    ads_factory_ = nullptr;
    ads_service_.reset();
  }

 private:
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  TestingPrefServiceSimple prefs_;
  raw_ptr<test::FakeAdsFactory> ads_factory_;
  std::unique_ptr<AdsServiceImplIOS> ads_service_;
};

TEST_F(BraveAdsServiceImplIOSTest,
       ClearsAdsDataWhenSponsoredAdsBecomeDisabled) {
  // Arrange
  prefs().SetBoolean(prefs::kSponsoredEnabled, true);
  // `ClearAdsPrefs` clears the whole `brave.brave_ads.*` prefix at once, so
  // checking this one pref is enough to tell whether the whole prefix was
  // cleared.
  prefs().SetString(prefs::kDiagnosticId, "foo");
  test::AdsServiceWaiter waiter(*ads_service());

  // Act
  prefs().SetBoolean(prefs::kSponsoredEnabled, false);
  waiter.WaitForOnDidClearAdsServiceData();

  // Assert
  EXPECT_FALSE(prefs().HasPrefPath(prefs::kDiagnosticId));
  EXPECT_FALSE(prefs().GetBoolean(prefs::kSponsoredEnabled));
}

TEST_F(BraveAdsServiceImplIOSTest,
       DoesNotClearAdsDataWhenUnrelatedPrefChanges) {
  // Arrange
  prefs().SetBoolean(prefs::kSponsoredEnabled, true);
  prefs().SetString(prefs::kDiagnosticId, "foo");

  // Act
  prefs().SetBoolean(prefs::kNotificationsEnabled, true);

  // Assert
  EXPECT_EQ("foo", prefs().GetString(prefs::kDiagnosticId));
}

TEST_F(BraveAdsServiceImplIOSTest,
       DoesNotClearAdsDataWhenSponsoredAdsAreEnabled) {
  // Arrange
  prefs().SetBoolean(prefs::kSponsoredEnabled, false);
  prefs().SetString(prefs::kDiagnosticId, "foo");

  // Act
  prefs().SetBoolean(prefs::kSponsoredEnabled, true);

  // Assert
  EXPECT_EQ("foo", prefs().GetString(prefs::kDiagnosticId));
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearsAdsDataOnEachSuccessiveSponsoredAdsDisable) {
  // Arrange
  prefs().SetBoolean(prefs::kSponsoredEnabled, true);
  prefs().SetString(prefs::kDiagnosticId, "foo");
  test::AdsServiceWaiter waiter1(*ads_service());

  // Act
  prefs().SetBoolean(prefs::kSponsoredEnabled, false);
  waiter1.WaitForOnDidClearAdsServiceData();

  prefs().SetBoolean(prefs::kSponsoredEnabled, true);
  prefs().SetString(prefs::kDiagnosticId, "bar");
  test::AdsServiceWaiter waiter2(*ads_service());
  prefs().SetBoolean(prefs::kSponsoredEnabled, false);
  waiter2.WaitForOnDidClearAdsServiceData();

  // Assert
  EXPECT_FALSE(prefs().HasPrefPath(prefs::kDiagnosticId));
  EXPECT_FALSE(prefs().GetBoolean(prefs::kSponsoredEnabled));
}

TEST_F(BraveAdsServiceImplIOSTest,
       KeyedServiceShutdownNotifiesObserversWhenNotInitialized) {
  // Arrange
  test::AdsServiceWaiter waiter(*ads_service());

  // Act
  static_cast<KeyedService&>(*ads_service()).Shutdown();

  // Assert
  waiter.WaitForOnDidShutdownAdsService();
}

TEST_F(BraveAdsServiceImplIOSTest,
       KeyedServiceShutdownNotifiesObserversWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  test::AdsServiceWaiter waiter(*ads_service());

  // Act
  static_cast<KeyedService&>(*ads_service()).Shutdown();

  // Assert
  waiter.WaitForOnDidShutdownAdsService();
  EXPECT_FALSE(ads_service()->IsInitialized());
}

TEST_F(BraveAdsServiceImplIOSTest,
       IsNotInitializedBeforeInitializeAdsIsCalled) {
  // Act & Assert
  EXPECT_FALSE(ads_service()->IsInitialized());
}

TEST_F(BraveAdsServiceImplIOSTest, NeverRequiresBrowserUpgradeToServeAds) {
  // Act & Assert
  EXPECT_FALSE(ads_service()->IsBrowserUpgradeRequiredToServeAds());
}

TEST_F(BraveAdsServiceImplIOSTest,
       GetMaximumNotificationAdsPerHourReturnsZero) {
  // Act & Assert
  EXPECT_EQ(0, ads_service()->GetMaximumNotificationAdsPerHour());
}

TEST_F(BraveAdsServiceImplIOSTest,
       GetAdsClientNotifierReturnsANonNullNotifier) {
  // Act & Assert
  EXPECT_TRUE(ads_service()->GetAdsClientNotifier());
}

TEST_F(BraveAdsServiceImplIOSTest,
       GetWeakPtrReturnsValidPointerToTheSameService) {
  // Act
  base::WeakPtr<AdsService> weak_ptr = ads_service()->GetWeakPtr();

  // Assert
  ASSERT_TRUE(weak_ptr);
  EXPECT_EQ(ads_service().get(), weak_ptr.get());
}

TEST_F(BraveAdsServiceImplIOSTest, GetWeakPtrIsInvalidatedAfterDestruction) {
  // Arrange
  base::WeakPtr<AdsService> weak_ptr = ads_service()->GetWeakPtr();
  ASSERT_TRUE(weak_ptr);

  // Act
  DestroyAdsService();

  // Assert
  EXPECT_FALSE(weak_ptr);
}

TEST_F(BraveAdsServiceImplIOSTest,
       DoesNotInitializeAdsWhenRewardsIsDisabledByPolicy) {
  // Arrange
  prefs().SetManagedPref(brave_rewards::prefs::kDisabledByPolicy,
                         base::Value(true));
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->InitializeAds(
      storage_path(), /*ads_client=*/nullptr, mojom::SysInfo::New(),
      mojom::BuildChannelInfo::New(), mojom::WalletInfo::New(),
      test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
  EXPECT_FALSE(ads_service()->IsInitialized());
}

TEST_F(BraveAdsServiceImplIOSTest,
       InitializeAdsSucceedsAndNotifiesObserversWhenAllowedToStart) {
  // Arrange
  NiceMock<AdsServiceObserverMock> observer;
  ads_service()->AddObserver(&observer);

  // Act & Assert
  EXPECT_CALL(observer, OnDidInitializeAdsService);
  EXPECT_TRUE(InitializeAdsSuccessfully());
  EXPECT_TRUE(ads_service()->IsInitialized());

  ads_service()->RemoveObserver(&observer);
}

TEST_F(BraveAdsServiceImplIOSTest,
       InitializeAdsFailsAndDoesNotNotifyObserversWhenEngineFailsToInitialize) {
  // Arrange
  ads_factory().set_simulate_initialization_failure();
  NiceMock<AdsServiceObserverMock> observer;
  ads_service()->AddObserver(&observer);
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(observer, OnDidInitializeAdsService).Times(0);
  ads_service()->InitializeAds(
      storage_path(), std::make_unique<test::FakeAdsClient>(),
      mojom::SysInfo::New(), mojom::BuildChannelInfo::New(),
      mojom::WalletInfo::New(), test_future.GetCallback());
  EXPECT_FALSE(test_future.Get());
  EXPECT_FALSE(ads_service()->IsInitialized());

  ads_service()->RemoveObserver(&observer);
}

TEST_F(BraveAdsServiceImplIOSTest, InitializeAdsFailsWhenAlreadyInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  ASSERT_EQ(1U, ads_factory().create_count());
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->InitializeAds(
      storage_path(), std::make_unique<test::FakeAdsClient>(),
      mojom::SysInfo::New(), mojom::BuildChannelInfo::New(),
      mojom::WalletInfo::New(), test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
  EXPECT_EQ(1U, ads_factory().create_count());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ShutdownAdsSucceedsImmediatelyWhenNotRunning) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->ShutdownAds(test_future.GetCallback());

  // Assert
  EXPECT_TRUE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ShutdownAdsSucceedsAndNotifiesObserversWhenRunning) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  NiceMock<AdsServiceObserverMock> observer;
  ads_service()->AddObserver(&observer);
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(observer, OnDidShutdownAdsService);
  ads_service()->ShutdownAds(test_future.GetCallback());
  EXPECT_TRUE(test_future.Get());
  EXPECT_FALSE(ads_service()->IsInitialized());

  ads_service()->RemoveObserver(&observer);
}

TEST_F(BraveAdsServiceImplIOSTest,
       MaybeGetNotificationAdReturnsNulloptWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<base::optional_ref<const NotificationAdInfo>>
      test_future;

  // Act
  ads_service()->MaybeGetNotificationAd("placement_id",
                                        test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       MaybeGetNotificationAdForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<base::optional_ref<const NotificationAdInfo>>
      test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), MaybeGetNotificationAd);
  ads_service()->MaybeGetNotificationAd("placement_id",
                                        test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       TriggerNotificationAdEventFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->TriggerNotificationAdEvent(
      "placement_id", mojom::NotificationAdEventType::kClicked,
      test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       TriggerNotificationAdEventForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), TriggerNotificationAdEvent);
  ads_service()->TriggerNotificationAdEvent(
      "placement_id", mojom::NotificationAdEventType::kClicked,
      test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       GetInternalsReturnsNulloptWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<std::optional<base::DictValue>> test_future;

  // Act
  ads_service()->GetInternals(test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       GetInternalsForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<std::optional<base::DictValue>> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), GetInternals);
  ads_service()->GetInternals(test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       GetDiagnosticsReturnsNulloptWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<std::optional<base::DictValue>> test_future;

  // Act
  ads_service()->GetDiagnostics(test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       GetDiagnosticsForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<std::optional<base::DictValue>> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), GetDiagnostics);
  ads_service()->GetDiagnostics(test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       EvaluateConditionMatcherReturnsUnknownWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<std::string, std::string> test_future;

  // Act
  ads_service()->EvaluateConditionMatcher("pref_path", "condition",
                                          /*test_value=*/std::nullopt,
                                          test_future.GetCallback());

  // Assert
  auto& [current_value, matches] = test_future.Get();
  EXPECT_EQ("Unknown", current_value);
  EXPECT_EQ("N/A", matches);
}

TEST_F(BraveAdsServiceImplIOSTest,
       EvaluateConditionMatcherForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<std::string, std::string> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), EvaluateConditionMatcher);
  ads_service()->EvaluateConditionMatcher("pref_path", "condition",
                                          /*test_value=*/std::nullopt,
                                          test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       GetStatementOfAccountsReturnsNullWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<mojom::StatementInfoPtr> test_future;

  // Act
  ads_service()->GetStatementOfAccounts(test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       GetStatementOfAccountsForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<mojom::StatementInfoPtr> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), GetStatementOfAccounts);
  ads_service()->GetStatementOfAccounts(test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ParseAndSaveNewTabPageAdsQueuesUntilServiceInitializes) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->ParseAndSaveNewTabPageAds(base::DictValue(),
                                           test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.IsReady());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ParseAndSaveNewTabPageAdsForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), ParseAndSaveNewTabPageAds);
  ads_service()->ParseAndSaveNewTabPageAds(base::DictValue(),
                                           test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       MaybeServeNewTabPageAdReturnsNullWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<mojom::NewTabPageAdInfoPtr> test_future;

  // Act
  ads_service()->MaybeServeNewTabPageAd(test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       MaybeServeNewTabPageAdForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<mojom::NewTabPageAdInfoPtr> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), MaybeServeNewTabPageAd);
  ads_service()->MaybeServeNewTabPageAd(test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       TriggerNewTabPageAdEventFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->TriggerNewTabPageAdEvent(
      "placement_id", "creative_instance_id",
      mojom::NewTabPageAdMetricType::kConfirmation,
      mojom::NewTabPageAdEventType::kClicked, test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       TriggerNewTabPageAdEventForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), TriggerNewTabPageAdEvent);
  ads_service()->TriggerNewTabPageAdEvent(
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
  ads_service()->MaybeGetSearchResultAd("placement_id",
                                        test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       MaybeGetSearchResultAdForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<mojom::CreativeSearchResultAdInfoPtr> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), MaybeGetSearchResultAd);
  ads_service()->MaybeGetSearchResultAd("placement_id",
                                        test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       TriggerSearchResultAdEventFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->TriggerSearchResultAdEvent(
      mojom::CreativeSearchResultAdInfo::New(),
      mojom::SearchResultAdEventType::kClicked, test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       TriggerSearchResultAdEventForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), TriggerSearchResultAdEvent);
  ads_service()->TriggerSearchResultAdEvent(
      mojom::CreativeSearchResultAdInfo::New(),
      mojom::SearchResultAdEventType::kClicked, test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       PurgeOrphanedAdEventsForTypeFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->PurgeOrphanedAdEventsForType(mojom::AdType::kNewTabPageAd,
                                              test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       PurgeOrphanedAdEventsForTypeForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), PurgeOrphanedAdEventsForType);
  ads_service()->PurgeOrphanedAdEventsForType(mojom::AdType::kNewTabPageAd,
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
  ads_service()->GetAdHistory(base::Time(), base::Time::Now(),
                              test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       GetAdHistoryForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<
      std::optional<std::vector<mojom::AdHistoryItemInfoPtr>>>
      test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), GetAdHistory);
  ads_service()->GetAdHistory(base::Time(), base::Time::Now(),
                              test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest, ToggleLikeAdFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->ToggleLikeAd(mojom::ReactionInfo::New(),
                              test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ToggleLikeAdForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), ToggleLikeAd);
  ads_service()->ToggleLikeAd(mojom::ReactionInfo::New(),
                              test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest, ToggleDislikeAdFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->ToggleDislikeAd(mojom::ReactionInfo::New(),
                                 test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ToggleDislikeAdForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), ToggleDislikeAd);
  ads_service()->ToggleDislikeAd(mojom::ReactionInfo::New(),
                                 test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest, ToggleLikeSegmentFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->ToggleLikeSegment(mojom::ReactionInfo::New(),
                                   test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ToggleLikeSegmentForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), ToggleLikeSegment);
  ads_service()->ToggleLikeSegment(mojom::ReactionInfo::New(),
                                   test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ToggleDislikeSegmentFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->ToggleDislikeSegment(mojom::ReactionInfo::New(),
                                      test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ToggleDislikeSegmentForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), ToggleDislikeSegment);
  ads_service()->ToggleDislikeSegment(mojom::ReactionInfo::New(),
                                      test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest, ToggleSaveAdFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->ToggleSaveAd(mojom::ReactionInfo::New(),
                              test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ToggleSaveAdForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), ToggleSaveAd);
  ads_service()->ToggleSaveAd(mojom::ReactionInfo::New(),
                              test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ToggleMarkAdAsInappropriateFailsWhenNotInitialized) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->ToggleMarkAdAsInappropriate(mojom::ReactionInfo::New(),
                                             test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ToggleMarkAdAsInappropriateForwardsToUnderlyingAdsWhenInitialized) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  base::test::TestFuture<bool> test_future;

  // Act & Assert
  EXPECT_CALL(GetAds(), ToggleMarkAdAsInappropriate);
  ads_service()->ToggleMarkAdAsInappropriate(mojom::ReactionInfo::New(),
                                             test_future.GetCallback());
  ASSERT_TRUE(test_future.Wait());
}

TEST_F(BraveAdsServiceImplIOSTest,
       NotifyDidInitializeAdsServiceNotifiesObservers) {
  // Arrange
  NiceMock<AdsServiceObserverMock> observer;
  ads_service()->AddObserver(&observer);

  // Act & Assert
  EXPECT_CALL(observer, OnDidInitializeAdsService);
  ads_service()->NotifyDidInitializeAdsService();

  ads_service()->RemoveObserver(&observer);
}

TEST_F(BraveAdsServiceImplIOSTest,
       NotifyDidShutdownAdsServiceNotifiesObservers) {
  // Arrange
  NiceMock<AdsServiceObserverMock> observer;
  ads_service()->AddObserver(&observer);

  // Act & Assert
  EXPECT_CALL(observer, OnDidShutdownAdsService);
  ads_service()->NotifyDidShutdownAdsService();

  ads_service()->RemoveObserver(&observer);
}

TEST_F(BraveAdsServiceImplIOSTest, RemovedObserverIsNotNotified) {
  // Arrange
  StrictMock<AdsServiceObserverMock> observer;
  ads_service()->AddObserver(&observer);
  ads_service()->RemoveObserver(&observer);

  // Act
  ads_service()->NotifyDidInitializeAdsService();
  ads_service()->NotifyDidShutdownAdsService();
  ads_service()->NotifyDidClearAdsServiceData();
}

TEST_F(BraveAdsServiceImplIOSTest, ClearDataClearsAdsPrefs) {
  // Arrange
  prefs().SetString(prefs::kDiagnosticId, "foo");
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->ClearData(test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_FALSE(prefs().HasPrefPath(prefs::kDiagnosticId));
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearDataPreservesSponsoredEnabledPrefWhenSponsoredAdsAreDisabled) {
  // Arrange
  prefs().SetBoolean(prefs::kSponsoredEnabled, false);
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->ClearData(test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_FALSE(prefs().GetBoolean(prefs::kSponsoredEnabled));
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearDataPreservesSponsoredEnabledPrefWhenSponsoredAdsAreEnabled) {
  // Arrange
  prefs().SetBoolean(prefs::kSponsoredEnabled, true);
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->ClearData(test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_TRUE(prefs().GetBoolean(prefs::kSponsoredEnabled));
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearDataDoesNotSetSponsoredEnabledPrefWhenUnset) {
  // Arrange
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->ClearData(test_future.GetCallback());
  ASSERT_TRUE(test_future.Get());

  // Assert
  EXPECT_FALSE(prefs().HasPrefPath(prefs::kSponsoredEnabled));
}

TEST_F(BraveAdsServiceImplIOSTest, ClearDataNotifiesObserversWhenNotRunning) {
  // Arrange
  test::AdsServiceWaiter waiter(*ads_service());
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->ClearData(test_future.GetCallback());
  waiter.WaitForOnDidClearAdsServiceData();

  // Assert
  EXPECT_TRUE(test_future.Get());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearDataRestartsServiceWhenItWasRunningBeforeClear) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  ASSERT_EQ(1U, ads_factory().create_count());
  base::WeakPtr<AdsMock> old_ads = GetAds().GetWeakPtr();
  base::test::TestFuture<bool> clear_future;

  // Act
  ads_service()->ClearData(clear_future.GetCallback());

  // Assert
  EXPECT_TRUE(clear_future.Get());
  EXPECT_FALSE(old_ads);
  EXPECT_TRUE(ads_service()->IsInitialized());
  EXPECT_EQ(2U, ads_factory().create_count());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearDataSkipsRestartWhenAlreadyReinitializedDuringClear) {
  // Arrange
  ASSERT_TRUE(InitializeAdsSuccessfully());
  ASSERT_EQ(1U, ads_factory().create_count());
  base::test::TestFuture<bool> clear_future;

  // Act
  ads_service()->ClearData(clear_future.GetCallback());
  base::test::TestFuture<bool> reinit_future;
  ads_service()->InitializeAds(
      storage_path(), std::make_unique<test::FakeAdsClient>(),
      mojom::SysInfo::New(), mojom::BuildChannelInfo::New(),
      mojom::WalletInfo::New(), reinit_future.GetCallback());
  ASSERT_TRUE(reinit_future.Get());
  ASSERT_EQ(2U, ads_factory().create_count());

  // Assert
  EXPECT_TRUE(clear_future.Get());
  EXPECT_TRUE(ads_service()->IsInitialized());
  EXPECT_EQ(2U, ads_factory().create_count());
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearDataFailsAndPreservesPrefsWhenUnderlyingShutdownFails) {
  // Arrange
  ads_factory().set_simulate_shutdown_failure();
  ASSERT_TRUE(InitializeAdsSuccessfully());
  prefs().SetString(prefs::kDiagnosticId, "foo");
  base::test::TestFuture<bool> test_future;

  // Act
  ads_service()->ClearData(test_future.GetCallback());

  // Assert
  EXPECT_FALSE(test_future.Get());
  EXPECT_EQ("foo", prefs().GetString(prefs::kDiagnosticId));
}

TEST_F(
    BraveAdsServiceImplIOSTest,
    PreservesAdsDataWhenSponsoredAdsBecomeDisabledForBraveRewardsUser) {
  // Arrange
  prefs().SetBoolean(prefs::kSponsoredEnabled, true);
  prefs().SetBoolean(brave_rewards::prefs::kEnabled, true);
  prefs().SetString(prefs::kDiagnosticId, "foo");

  // Act
  prefs().SetBoolean(prefs::kSponsoredEnabled, false);
  // Ensures `MaybeClearAdsData`'s posted task has had a
  // chance to (not) clear data before asserting.
  FlushPendingTasks();

  // Assert
  EXPECT_EQ("foo", prefs().GetString(prefs::kDiagnosticId));
}

TEST_F(
    BraveAdsServiceImplIOSTest,
    PreservesAdsDataWhenBraveRewardsBecomesDisabledWhileSponsoredAdsRemainEnabled) {
  // Arrange
  prefs().SetBoolean(prefs::kSponsoredEnabled, true);
  prefs().SetBoolean(brave_rewards::prefs::kEnabled, true);
  prefs().SetString(prefs::kDiagnosticId, "foo");

  // Act
  prefs().SetBoolean(brave_rewards::prefs::kEnabled, false);
  // Ensures `MaybeClearAdsData`'s posted task has had a
  // chance to (not) clear data before asserting.
  FlushPendingTasks();

  // Assert
  EXPECT_EQ("foo", prefs().GetString(prefs::kDiagnosticId));
}

TEST_F(BraveAdsServiceImplIOSTest,
       ClearsAdsDataWhenBraveRewardsBecomesDisabledAndSponsoredAdsAreDisabled) {
  // Arrange
  prefs().SetBoolean(brave_rewards::prefs::kEnabled, true);
  prefs().SetBoolean(prefs::kSponsoredEnabled, false);
  // `ClearAdsPrefs` clears the whole `brave.brave_ads.*` prefix at once, so
  // checking this one pref is enough to tell whether the whole prefix was
  // cleared.
  prefs().SetString(prefs::kDiagnosticId, "foo");
  test::AdsServiceWaiter waiter(*ads_service());

  // Act
  prefs().SetBoolean(brave_rewards::prefs::kEnabled, false);
  waiter.WaitForOnDidClearAdsServiceData();

  // Assert
  EXPECT_FALSE(prefs().HasPrefPath(prefs::kDiagnosticId));
}

}  // namespace brave_ads
