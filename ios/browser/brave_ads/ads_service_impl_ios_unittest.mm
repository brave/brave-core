// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_ads/ads_service_impl_ios.h"

#include "base/functional/callback_helpers.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/browser/service/test/ads_service_waiter.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace brave_ads {

class AdsServiceImplIOSTest : public PlatformTest {
 public:
  AdsServiceImplIOSTest() {
    RegisterProfilePrefs(prefs_.registry());
    prefs_.registry()->RegisterBooleanPref(brave_rewards::prefs::kEnabled,
                                           false);
    prefs_.registry()->RegisterBooleanPref(
        brave_rewards::prefs::kDisabledByPolicy, false);
    ads_service_ = std::make_unique<AdsServiceImplIOS>(prefs_);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  TestingPrefServiceSimple prefs_;
  std::unique_ptr<AdsServiceImplIOS> ads_service_;
};

TEST_F(AdsServiceImplIOSTest, ClearsAdsDataWhenSponsoredAdsBecomeDisabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  // A proxy for the `brave.brave_ads.*` prefs cleared alongside it.
  prefs_.SetString(prefs::kDiagnosticId, "foo");
  test::AdsServiceWaiter waiter(*ads_service_);

  // Act
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  waiter.WaitForOnDidClearAdsServiceData();

  // Assert
  EXPECT_FALSE(prefs_.HasPrefPath(prefs::kDiagnosticId));
  EXPECT_FALSE(prefs_.GetBoolean(prefs::kSponsoredEnabled));
}

TEST_F(AdsServiceImplIOSTest, DoesNotClearAdsDataWhenUnrelatedPrefChanges) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  prefs_.SetString(prefs::kDiagnosticId, "foo");

  // Act
  prefs_.SetBoolean(prefs::kNotificationsEnabled, true);

  // Assert
  EXPECT_EQ("foo", prefs_.GetString(prefs::kDiagnosticId));
}

TEST_F(AdsServiceImplIOSTest, DoesNotClearAdsDataWhenSponsoredAdsAreEnabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  prefs_.SetString(prefs::kDiagnosticId, "foo");

  // Act
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);

  // Assert
  EXPECT_EQ("foo", prefs_.GetString(prefs::kDiagnosticId));
}

TEST_F(AdsServiceImplIOSTest, EligibleToStartWhenOnlySponsoredAdsAreEnabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);

  // Act
  ads_service_->Init(
      /*storage_path=*/"", /*ads_client=*/nullptr, mojom::SysInfo::New(),
      mojom::BuildChannelInfo::New(), /*mojom_wallet=*/nullptr,
      base::DoNothing());

  // Assert
  EXPECT_FALSE(ads_service_->IsIneligibleToStart());
}

TEST_F(AdsServiceImplIOSTest, EligibleToStartWhenNotificationAdsAreEnabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
  prefs_.SetBoolean(prefs::kNotificationsEnabled, true);

  // Act
  ads_service_->Init(
      /*storage_path=*/"", /*ads_client=*/nullptr, mojom::SysInfo::New(),
      mojom::BuildChannelInfo::New(), /*mojom_wallet=*/nullptr,
      base::DoNothing());

  // Assert
  EXPECT_FALSE(ads_service_->IsIneligibleToStart());
}

TEST_F(AdsServiceImplIOSTest, IneligibleToStartWhenAllAdTypesAreDisabled) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
  test::AdsServiceWaiter waiter(*ads_service_);

  // Act
  bool success = true;
  ads_service_->Init(
      /*storage_path=*/"", /*ads_client=*/nullptr, mojom::SysInfo::New(),
      mojom::BuildChannelInfo::New(), /*mojom_wallet=*/nullptr,
      base::BindLambdaForTesting(
          [&success](bool result) { success = result; }));

  // Assert
  waiter.WaitForOnAdsServiceIneligibleToStart();
  EXPECT_FALSE(success);
  EXPECT_TRUE(ads_service_->IsIneligibleToStart());
}

TEST_F(AdsServiceImplIOSTest,
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

TEST_F(AdsServiceImplIOSTest,
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

TEST_F(AdsServiceImplIOSTest, IneligibleToStartWhenRewardsIsDisabledByPolicy) {
  // Arrange
  prefs_.SetManagedPref(brave_rewards::prefs::kDisabledByPolicy,
                        base::Value(true));
  prefs_.SetBoolean(brave_rewards::prefs::kEnabled, true);
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);
  prefs_.SetBoolean(prefs::kNotificationsEnabled, true);
  test::AdsServiceWaiter waiter(*ads_service_);

  // Act
  bool success = true;
  ads_service_->Init(
      /*storage_path=*/"", /*ads_client=*/nullptr, mojom::SysInfo::New(),
      mojom::BuildChannelInfo::New(), /*mojom_wallet=*/nullptr,
      base::BindLambdaForTesting(
          [&success](bool result) { success = result; }));

  // Assert
  waiter.WaitForOnAdsServiceIneligibleToStart();
  EXPECT_FALSE(success);
  EXPECT_TRUE(ads_service_->IsIneligibleToStart());
}

}  // namespace brave_ads
