/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#import "brave/ios/browser/api/ads/brave_ads.h"

#import <Foundation/Foundation.h>

#include <optional>
#include <string>

#include "base/check.h"
#include "base/files/file.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/raw_ptr.h"
#include "base/strings/sys_string_conversions.h"
#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_util.h"
#include "brave/components/brave_ads/core/public/history/site_history.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_news/common/pref_names.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "brave/components/ntp_background_images/browser/sponsored_content/new_tab_takeover/new_tab_takeover_infobar_util.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#import "brave/ios/browser/api/ads/ads_client_bridge.h"
#include "components/prefs/pref_service.h"
#include "ios/chrome/browser/shared/model/application_context/application_context.h"
#include "ios/chrome/browser/shared/model/profile/profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/test/test_profile_ios.h"
#include "ios/chrome/browser/shared/model/profile/test/test_profile_manager_ios.h"
#include "ios/chrome/test/ios_chrome_scoped_testing_local_state.h"
#include "ios/web/public/test/web_task_environment.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/gtest_mac.h"
#include "testing/platform_test.h"

// Declares anonymous methods of `BraveAds` to make them visible for tests.
@interface BraveAds (Testing)
- (BOOL)registerAdsResourcesForLanguageCode:(NSString*)languageCode;
- (BOOL)registerAdsResourcesForCountryCode:(NSString*)countryCode;
@end

@interface FakeBraveAdsNotificationHandler
    : NSObject <BraveAdsNotificationHandler>
@property(nonatomic) BOOL canShowNotificationAdsValue;
@property(nonatomic) BraveAdsNotificationAdInfo* lastShownNotificationAd;
@property(nonatomic, copy) NSString* lastClosedPlacementId;
@end

@implementation FakeBraveAdsNotificationHandler
- (BOOL)canShowNotificationAds {
  return self.canShowNotificationAdsValue;
}
- (void)showNotificationAd:(BraveAdsNotificationAdInfo*)ad {
  self.lastShownNotificationAd = ad;
}
- (void)closeNotificationAd:(NSString*)placementId {
  self.lastClosedPlacementId = placementId;
}
@end

@interface FakeBraveAdsCaptchaHandler : NSObject <BraveAdsCaptchaHandler>
@property(nonatomic, copy) NSString* lastPaymentId;
@property(nonatomic, copy) NSString* lastCaptchaId;
@end

@implementation FakeBraveAdsCaptchaHandler
- (void)handleAdaptiveCaptchaForPaymentId:(NSString*)paymentId
                                captchaId:(NSString*)captchaId {
  self.lastPaymentId = paymentId;
  self.lastCaptchaId = captchaId;
}
@end

class BraveAdsTest : public PlatformTest {
 public:
  BraveAdsTest() {
    CHECK(temp_dir_.CreateUniqueTempDir());
    profile_ =
        profile_manager_.AddProfileWithBuilder(TestProfileIOS::Builder());
    ads_ = [[BraveAds alloc]
        initWithStateStoragePath:base::SysUTF8ToNSString(
                                     temp_dir_.GetPath().value())];
  }

  PrefService* profile_prefs() { return profile_->GetPrefs(); }

  PrefService* local_state_prefs() {
    return GetApplicationContext()->GetLocalState();
  }

  id<AdsClientBridge> bridge() {
    return static_cast<id<AdsClientBridge>>(ads_);
  }

 protected:
  web::WebTaskEnvironment task_environment_;
  IOSChromeScopedTestingLocalState scoped_testing_local_state_;
  TestProfileManagerIOS profile_manager_;
  raw_ptr<ProfileIOS> profile_ = nullptr;
  base::ScopedTempDir temp_dir_;
  BraveAds* ads_;
};

TEST_F(BraveAdsTest, IsServiceRunningReturnsFalseBeforeInitialization) {
  // Act & Assert
  EXPECT_FALSE([ads_ isServiceRunning]);
}

TEST_F(BraveAdsTest, IsSupportedRegionMatchesUnderlyingAdsUtil) {
  // Act & Assert
  EXPECT_EQ(brave_ads::IsSupportedRegion(), [BraveAds isSupportedRegion]);
}

TEST_F(BraveAdsTest,
       ShouldShowSearchResultAdClickedInfoBarReturnsPrefDefaultValue) {
  // Act & Assert
  EXPECT_TRUE([ads_ shouldShowSearchResultAdClickedInfoBar]);
}

TEST_F(BraveAdsTest,
       ShouldShowSearchResultAdClickedInfoBarReflectsUpdatedPrefValue) {
  // Arrange
  profile_prefs()->SetBoolean(
      brave_ads::prefs::kShouldShowSearchResultAdClickedInfoBar, false);

  // Act & Assert
  EXPECT_FALSE([ads_ shouldShowSearchResultAdClickedInfoBar]);
}

TEST_F(BraveAdsTest, ShouldDisplayNewTabTakeoverInfobarReturnsTrueByDefault) {
  // Act & Assert
  EXPECT_TRUE([ads_ shouldDisplayNewTabTakeoverInfobar]);
}

TEST_F(BraveAdsTest,
       ShouldDisplayNewTabTakeoverInfobarReturnsFalseWhenRewardsEnabled) {
  // Arrange
  profile_prefs()->SetBoolean(brave_rewards::prefs::kEnabled, true);

  // Act & Assert
  EXPECT_FALSE([ads_ shouldDisplayNewTabTakeoverInfobar]);
}

TEST_F(BraveAdsTest,
       RecordNewTabTakeoverInfobarWasDisplayedDecrementsRemainingCount) {
  // Act
  [ads_ recordNewTabTakeoverInfobarWasDisplayed];

  // Assert
  EXPECT_EQ(4, profile_prefs()->GetInteger(
                   ntp_background_images::prefs::
                       kNewTabTakeoverInfobarRemainingDisplayCount));
}

TEST_F(BraveAdsTest,
       SuppressNewTabTakeoverInfobarImmediatelyPreventsFutureDisplay) {
  // Act
  [ads_ suppressNewTabTakeoverInfobar];

  // Assert
  EXPECT_EQ(0, profile_prefs()->GetInteger(
                   ntp_background_images::prefs::
                       kNewTabTakeoverInfobarRemainingDisplayCount));
  EXPECT_FALSE([ads_ shouldDisplayNewTabTakeoverInfobar]);
}

TEST_F(BraveAdsTest,
       NotifyBraveNewsIsEnabledPreferenceDidChangeSetsPrefsWhenEnabled) {
  // Act
  [ads_ notifyBraveNewsIsEnabledPreferenceDidChange:YES];

  // Assert
  EXPECT_TRUE(
      profile_prefs()->GetBoolean(brave_news::prefs::kBraveNewsOptedIn));
  EXPECT_TRUE(
      profile_prefs()->GetBoolean(brave_news::prefs::kNewTabPageShowToday));
}

TEST_F(BraveAdsTest,
       NotifyBraveNewsIsEnabledPreferenceDidChangeSetsPrefsWhenDisabled) {
  // Arrange
  [ads_ notifyBraveNewsIsEnabledPreferenceDidChange:YES];

  // Act
  [ads_ notifyBraveNewsIsEnabledPreferenceDidChange:NO];

  // Assert
  EXPECT_FALSE(
      profile_prefs()->GetBoolean(brave_news::prefs::kBraveNewsOptedIn));
  EXPECT_TRUE(
      profile_prefs()->HasPrefPath(brave_news::prefs::kBraveNewsOptedIn));
  EXPECT_FALSE(
      profile_prefs()->GetBoolean(brave_news::prefs::kNewTabPageShowToday));
  EXPECT_TRUE(
      profile_prefs()->HasPrefPath(brave_news::prefs::kNewTabPageShowToday));
}

TEST_F(BraveAdsTest,
       NotifySponsoredImagesIsEnabledPreferenceDidChangeSetsPrefsWhenEnabled) {
  // Arrange
  profile_prefs()->SetBoolean(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage, false);
  profile_prefs()->SetBoolean(brave_ads::prefs::kSponsoredEnabled, false);

  // Act
  [ads_ notifySponsoredImagesIsEnabledPreferenceDidChange:YES];

  // Assert
  EXPECT_TRUE(profile_prefs()->GetBoolean(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage));
  EXPECT_TRUE(profile_prefs()->GetBoolean(brave_ads::prefs::kSponsoredEnabled));
}

TEST_F(BraveAdsTest,
       NotifySponsoredImagesIsEnabledPreferenceDidChangeSetsPrefsWhenDisabled) {
  // Act
  [ads_ notifySponsoredImagesIsEnabledPreferenceDidChange:NO];

  // Assert
  EXPECT_FALSE(profile_prefs()->GetBoolean(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage));
  EXPECT_TRUE(profile_prefs()->HasPrefPath(
      ntp_background_images::prefs::kNewTabPageShowBackgroundImage));
  EXPECT_FALSE(
      profile_prefs()->GetBoolean(brave_ads::prefs::kSponsoredEnabled));
  EXPECT_TRUE(
      profile_prefs()->HasPrefPath(brave_ads::prefs::kSponsoredEnabled));
}

TEST_F(BraveAdsTest, IsEnabledDefaultsToFalse) {
  // Act & Assert
  EXPECT_FALSE([ads_ isEnabled]);
}

TEST_F(BraveAdsTest, SetEnabledTogglesRewardsAndNotificationsPrefsWhenTrue) {
  // Act
  ads_.enabled = YES;

  // Assert
  EXPECT_TRUE(profile_prefs()->GetBoolean(brave_rewards::prefs::kEnabled));
  EXPECT_TRUE(
      profile_prefs()->GetBoolean(brave_ads::prefs::kNotificationsEnabled));
}

TEST_F(BraveAdsTest, SetEnabledTogglesRewardsAndNotificationsPrefsWhenFalse) {
  // Arrange
  ads_.enabled = YES;

  // Act
  ads_.enabled = NO;

  // Assert
  EXPECT_FALSE(profile_prefs()->GetBoolean(brave_rewards::prefs::kEnabled));
  EXPECT_FALSE(
      profile_prefs()->GetBoolean(brave_ads::prefs::kNotificationsEnabled));
}

TEST_F(BraveAdsTest,
       GetStatementOfAccountsReturnsZeroValuesWhenServiceNotRunning) {
  // Act
  base::test::TestFuture<NSInteger, double, NSDate*> test_future;
  auto* test_future_ptr = &test_future;
  [ads_
      getStatementOfAccounts:^(NSInteger adsReceived, double estimatedEarnings,
                               NSDate* nextPaymentDate) {
        test_future_ptr->SetValue(adsReceived, estimatedEarnings,
                                   nextPaymentDate);
      }];
  const auto& [ads_received, estimated_earnings, next_payment_date] =
      test_future.Get();

  // Assert
  EXPECT_EQ(0, ads_received);
  EXPECT_DOUBLE_EQ(0, estimated_earnings);
  EXPECT_NSEQ(nil, next_payment_date);
}

TEST_F(BraveAdsTest, MaybeServeNewTabPageAdReturnsNilWhenServiceNotRunning) {
  // Act & Assert
  base::test::TestFuture<BraveAdsNewTabPageAdInfo*> test_future;
  auto* test_future_ptr = &test_future;
  [ads_ maybeServeNewTabPageAd:^(BraveAdsNewTabPageAdInfo* ad) {
    test_future_ptr->SetValue(ad);
  }];
  EXPECT_NSEQ(nil, test_future.Get());
}

TEST_F(BraveAdsTest,
       TriggerNewTabPageAdEventReturnsFailureWhenServiceNotRunning) {
  // Act & Assert
  base::test::TestFuture<BOOL> test_future;
  auto* test_future_ptr = &test_future;
  [ads_ triggerNewTabPageAdEvent:@"wallpaper-id"
              creativeInstanceId:@"creative-instance-id"
                      metricType:BraveAdsNewTabPageAdMetricTypeConfirmation
                       eventType:BraveAdsNewTabPageAdEventTypeClicked
                      completion:^(BOOL success) {
                        test_future_ptr->SetValue(success);
                      }];
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsTest, MaybeGetNotificationAdReturnsNilWhenServiceNotRunning) {
  // Act & Assert
  base::test::TestFuture<BraveAdsNotificationAdInfo*> test_future;
  auto* test_future_ptr = &test_future;
  [ads_ maybeGetNotificationAd:@"placement-id"
                    completion:^(BraveAdsNotificationAdInfo* ad) {
                      test_future_ptr->SetValue(ad);
                    }];
  EXPECT_NSEQ(nil, test_future.Get());
}

TEST_F(BraveAdsTest,
       TriggerNotificationAdEventReturnsFailureWhenServiceNotRunning) {
  // Act & Assert
  base::test::TestFuture<BOOL> test_future;
  auto* test_future_ptr = &test_future;
  [ads_ triggerNotificationAdEvent:@"placement-id"
                         eventType:BraveAdsNotificationAdEventTypeClicked
                        completion:^(BOOL success) {
                          test_future_ptr->SetValue(success);
                        }];
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsTest,
       TriggerSearchResultAdClickedEventReturnsFailureWhenServiceNotRunning) {
  // Act & Assert
  base::test::TestFuture<BOOL> test_future;
  auto* test_future_ptr = &test_future;
  [ads_ triggerSearchResultAdClickedEvent:@"placement-id"
                               completion:^(BOOL success) {
                                 test_future_ptr->SetValue(success);
                               }];
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsTest,
       TriggerSearchResultAdViewedEventReturnsFailureWhenServiceNotRunning) {
  // Act & Assert
  base::test::TestFuture<BOOL> test_future;
  auto* test_future_ptr = &test_future;
  BraveAdsCreativeSearchResultAdInfo* search_result_ad =
      [[BraveAdsCreativeSearchResultAdInfo alloc] init];
  [ads_ triggerSearchResultAdViewedEvent:search_result_ad
                              completion:^(BOOL success) {
                                test_future_ptr->SetValue(success);
                              }];
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsTest,
       PurgeOrphanedAdEventsForTypeReturnsFailureWhenServiceNotRunning) {
  // Act & Assert
  base::test::TestFuture<BOOL> test_future;
  auto* test_future_ptr = &test_future;
  [ads_ purgeOrphanedAdEventsForType:BraveAdsAdTypeNotificationAd
                          completion:^(BOOL success) {
                            test_future_ptr->SetValue(success);
                          }];
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsTest, ClearDataInvokesCompletionWhenServiceNotRunning) {
  // Act & Assert
  base::test::TestFuture<bool> test_future;
  auto* test_future_ptr = &test_future;
  [ads_ clearData:^{
    test_future_ptr->SetValue(true);
  }];
  EXPECT_TRUE(test_future.Get());
}

TEST_F(BraveAdsTest,
       ShutdownServiceInvokesCompletionImmediatelyWhenServiceNotRunning) {
  // Act & Assert
  base::test::TestFuture<bool> test_future;
  auto* test_future_ptr = &test_future;
  [ads_ shutdownService:^{
    test_future_ptr->SetValue(true);
  }];
  EXPECT_TRUE(test_future.Get());
}

TEST_F(BraveAdsTest, ShutdownServiceHandlesNilCompletionWhenServiceNotRunning) {
  // Act & Assert
  [ads_ shutdownService:nil];
}

TEST_F(BraveAdsTest, CanShowNotificationAdsForwardsToNotificationsHandler) {
  // Arrange
  FakeBraveAdsNotificationHandler* notification_handler =
      [[FakeBraveAdsNotificationHandler alloc] init];
  notification_handler.canShowNotificationAdsValue = YES;
  ads_.notificationsHandler = notification_handler;

  // Act & Assert
  EXPECT_TRUE([bridge() canShowNotificationAds]);
}

TEST_F(BraveAdsTest, ShowNotificationAdForwardsToNotificationsHandler) {
  // Arrange
  FakeBraveAdsNotificationHandler* notification_handler =
      [[FakeBraveAdsNotificationHandler alloc] init];
  ads_.notificationsHandler = notification_handler;

  // Act
  [bridge() showNotificationAd:brave_ads::mojom::NotificationAdInfo::New()];

  // Assert
  EXPECT_NE(nil, notification_handler.lastShownNotificationAd);
}

TEST_F(BraveAdsTest, CloseNotificationAdForwardsToNotificationsHandler) {
  // Arrange
  FakeBraveAdsNotificationHandler* notification_handler =
      [[FakeBraveAdsNotificationHandler alloc] init];
  ads_.notificationsHandler = notification_handler;

  // Act
  [bridge() closeNotificationAd:std::string("placement-id")];

  // Assert
  EXPECT_NSEQ(@"placement-id", notification_handler.lastClosedPlacementId);
}

TEST_F(BraveAdsTest, ShowScheduledCaptchaForwardsToCaptchaHandler) {
  // Arrange
  FakeBraveAdsCaptchaHandler* captcha_handler =
      [[FakeBraveAdsCaptchaHandler alloc] init];
  ads_.captchaHandler = captcha_handler;

  // Act
  [bridge() showScheduledCaptcha:std::string("payment-id")
                       captchaId:std::string("captcha-id")];

  // Assert
  EXPECT_NSEQ(@"payment-id", captcha_handler.lastPaymentId);
  EXPECT_NSEQ(@"captcha-id", captcha_handler.lastCaptchaId);
}

TEST_F(BraveAdsTest, IsBrowserInFullScreenModeAlwaysReturnsTrue) {
  // Act & Assert
  EXPECT_TRUE([bridge() isBrowserInFullScreenMode]);
}

TEST_F(BraveAdsTest,
       CanShowNotificationAdsWhileBrowserIsBackgroundedAlwaysReturnsFalse) {
  // Act & Assert
  EXPECT_FALSE([bridge() canShowNotificationAdsWhileBrowserIsBackgrounded]);
}

TEST_F(BraveAdsTest, GetSiteHistoryAlwaysReturnsEmptyResult) {
  // Act
  base::test::TestFuture<brave_ads::SiteHistoryList> test_future;
  [bridge()
      getSiteHistory:5
             forDays:7
            callback:base::BindOnce(
                         [](base::test::TestFuture<brave_ads::SiteHistoryList>*
                                future,
                            const brave_ads::SiteHistoryList& history) {
                           future->SetValue(history);
                         },
                         &test_future)];

  // Assert
  EXPECT_TRUE(test_future.Get().empty());
}

TEST_F(BraveAdsTest, GetProfilePrefReturnsDefaultValueForPrefThatWasNeverSet) {
  // Act
  std::optional<base::Value> stored_value = [bridge()
      getProfilePref:brave_ads::prefs::kShouldShowSearchResultAdClickedInfoBar];

  // Assert
  EXPECT_EQ(base::Value(true), stored_value);
}

TEST_F(BraveAdsTest,
       SetProfilePrefPersistsAndGetProfilePrefReturnsStoredValue) {
  // Act
  [bridge()
      setProfilePref:brave_ads::prefs::kShouldShowSearchResultAdClickedInfoBar
               value:base::Value(false)];
  std::optional<base::Value> stored_value = [bridge()
      getProfilePref:brave_ads::prefs::kShouldShowSearchResultAdClickedInfoBar];

  // Assert
  EXPECT_EQ(base::Value(false), stored_value);
  EXPECT_FALSE(profile_prefs()->GetBoolean(
      brave_ads::prefs::kShouldShowSearchResultAdClickedInfoBar));
}

TEST_F(BraveAdsTest, FindProfilePrefReturnsFalseForUnregisteredPrefPath) {
  // Act & Assert
  EXPECT_FALSE([bridge()
      findProfilePref:std::string("brave.ads.nonexistent_pref")]);
}

TEST_F(BraveAdsTest, FindProfilePrefReturnsTrueForRegisteredPrefPath) {
  // Act & Assert
  EXPECT_TRUE(
      [bridge() findProfilePref:brave_ads::prefs::
                                    kShouldShowSearchResultAdClickedInfoBar]);
}

TEST_F(BraveAdsTest, HasProfilePrefPathReturnsFalseForUnsetPref) {
  // Act & Assert
  EXPECT_FALSE(
      [bridge() hasProfilePrefPath:
                    brave_ads::prefs::kShouldShowSearchResultAdClickedInfoBar]);
}

TEST_F(BraveAdsTest, HasProfilePrefPathReturnsTrueAfterSettingPref) {
  // Arrange
  [bridge()
      setProfilePref:brave_ads::prefs::kShouldShowSearchResultAdClickedInfoBar
               value:base::Value(false)];

  // Act & Assert
  EXPECT_TRUE(
      [bridge() hasProfilePrefPath:
                    brave_ads::prefs::kShouldShowSearchResultAdClickedInfoBar]);
}

TEST_F(BraveAdsTest, ClearProfilePrefRemovesStoredValue) {
  // Arrange
  [bridge()
      setProfilePref:brave_ads::prefs::kShouldShowSearchResultAdClickedInfoBar
               value:base::Value(false)];

  // Act
  [bridge() clearProfilePref:brave_ads::prefs::
                                 kShouldShowSearchResultAdClickedInfoBar];

  // Assert
  EXPECT_FALSE(
      [bridge() hasProfilePrefPath:
                    brave_ads::prefs::kShouldShowSearchResultAdClickedInfoBar]);
}

TEST_F(BraveAdsTest,
       SetLocalStatePrefPersistsAndGetLocalStatePrefReturnsStoredValue) {
  // Act
  [bridge() setLocalStatePref:brave_ads::prefs::kObliviousHttpKeyConfig
                        value:base::Value("oblivious-http-key-config")];
  std::optional<base::Value> stored_value =
      [bridge() getLocalStatePref:brave_ads::prefs::kObliviousHttpKeyConfig];

  // Assert
  EXPECT_EQ(base::Value("oblivious-http-key-config"), stored_value);
  EXPECT_EQ("oblivious-http-key-config", local_state_prefs()->GetString(
                                   brave_ads::prefs::kObliviousHttpKeyConfig));
}

TEST_F(BraveAdsTest, FindLocalStatePrefReturnsFalseForUnregisteredPrefPath) {
  // Act & Assert
  EXPECT_FALSE([bridge()
      findLocalStatePref:std::string("brave.ads.nonexistent_pref")]);
}

TEST_F(BraveAdsTest, FindLocalStatePrefReturnsTrueForRegisteredPrefPath) {
  // Act & Assert
  EXPECT_TRUE(
      [bridge() findLocalStatePref:brave_ads::prefs::kObliviousHttpKeyConfig]);
}

TEST_F(BraveAdsTest, HasLocalStatePrefPathReturnsFalseForUnsetPref) {
  // Act & Assert
  EXPECT_FALSE([bridge()
      hasLocalStatePrefPath:brave_ads::prefs::kObliviousHttpKeyConfig]);
}

TEST_F(BraveAdsTest, HasLocalStatePrefPathReturnsTrueAfterSettingPref) {
  // Arrange
  [bridge() setLocalStatePref:brave_ads::prefs::kObliviousHttpKeyConfig
                        value:base::Value("oblivious-http-key-config")];

  // Act & Assert
  EXPECT_TRUE([bridge()
      hasLocalStatePrefPath:brave_ads::prefs::kObliviousHttpKeyConfig]);
}

TEST_F(BraveAdsTest, ClearLocalStatePrefRemovesStoredValue) {
  // Arrange
  [bridge() setLocalStatePref:brave_ads::prefs::kObliviousHttpKeyConfig
                        value:base::Value("oblivious-http-key-config")];

  // Act
  [bridge() clearLocalStatePref:brave_ads::prefs::kObliviousHttpKeyConfig];

  // Assert
  EXPECT_FALSE([bridge()
      hasLocalStatePrefPath:brave_ads::prefs::kObliviousHttpKeyConfig]);
}

TEST_F(BraveAdsTest, SaveThenLoadReturnsSavedContents) {
  // Arrange
  base::test::TestFuture<bool> save_test_future;
  [bridge() save:std::string("resource-name")
           value:std::string("foo")
        callback:save_test_future.GetCallback()];
  ASSERT_TRUE(save_test_future.Get());

  // Act
  base::test::TestFuture<std::optional<std::string>> load_test_future;
  [bridge() load:std::string("resource-name")
        callback:load_test_future
                     .GetCallback<const std::optional<std::string>&>()];

  // Assert
  EXPECT_THAT(load_test_future.Get(),
              ::testing::Optional(std::string("foo")));
}

TEST_F(BraveAdsTest, LoadReturnsNilForMissingFile) {
  // Act
  base::test::TestFuture<std::optional<std::string>> test_future;
  [bridge() load:std::string("missing-file")
        callback:test_future.GetCallback<const std::optional<std::string>&>()];

  // Assert
  EXPECT_FALSE(test_future.Get());
}

TEST_F(BraveAdsTest, RemoveDeletesSavedFile) {
  // Arrange
  base::test::TestFuture<bool> save_test_future;
  [bridge() save:std::string("resource-name")
           value:std::string("foo")
        callback:save_test_future.GetCallback()];
  ASSERT_TRUE(save_test_future.Get());

  // Act
  base::test::TestFuture<bool> remove_test_future;
  [bridge() remove:std::string("resource-name")
          callback:remove_test_future.GetCallback()];
  ASSERT_TRUE(remove_test_future.Get());

  // Assert
  base::test::TestFuture<std::optional<std::string>> load_test_future;
  [bridge() load:std::string("resource-name")
        callback:load_test_future
                     .GetCallback<const std::optional<std::string>&>()];
  EXPECT_FALSE(load_test_future.Get());
}

TEST_F(BraveAdsTest, LoadResourceComponentReturnsValidFileForExistingResource) {
  // Arrange
  base::test::TestFuture<bool> save_test_future;
  [bridge() save:std::string("resource-id")
           value:std::string("foo")
        callback:save_test_future.GetCallback()];
  ASSERT_TRUE(save_test_future.Get());

  // Act
  base::test::TestFuture<base::File, bool> load_test_future;
  [bridge() loadResourceComponent:std::string("resource-id")
                          version:1
                         callback:load_test_future.GetCallback()];

  // Assert
  const auto& [file, exists] = load_test_future.Get();
  EXPECT_TRUE(file.IsValid());
  EXPECT_TRUE(exists);
}

TEST_F(BraveAdsTest,
       LoadResourceComponentReturnsInvalidFileForMissingResource) {
  // Act
  base::test::TestFuture<base::File, bool> test_future;
  [bridge() loadResourceComponent:std::string("missing-resource")
                          version:1
                         callback:test_future.GetCallback()];

  // Assert
  const auto& [file, exists] = test_future.Get();
  EXPECT_FALSE(file.IsValid());
  EXPECT_FALSE(exists);
}

TEST_F(BraveAdsTest,
       GetVirtualPrefsReturnsPopulatedDictionaryAfterConstruction) {
  // Act & Assert
  EXPECT_FALSE([bridge() getVirtualPrefs].empty());
}

TEST_F(BraveAdsTest,
       RegisterAdsResourcesForLanguageCodeReturnsFalseForNilCode) {
  // Act & Assert
  EXPECT_FALSE([ads_ registerAdsResourcesForLanguageCode:nil]);
}

TEST_F(BraveAdsTest,
       RegisterAdsResourcesForLanguageCodeReturnsFalseForUnknownCode) {
  // Act & Assert
  EXPECT_FALSE([ads_ registerAdsResourcesForLanguageCode:@"zz"]);
}

TEST_F(BraveAdsTest, RegisterAdsResourcesForCountryCodeReturnsFalseForNilCode) {
  // Act & Assert
  EXPECT_FALSE([ads_ registerAdsResourcesForCountryCode:nil]);
}

TEST_F(BraveAdsTest,
       RegisterAdsResourcesForCountryCodeReturnsFalseForUnknownCode) {
  // Act & Assert
  EXPECT_FALSE([ads_ registerAdsResourcesForCountryCode:@"zz"]);
}
