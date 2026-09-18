// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_ads/ads_service_impl_ios.h"

#include <string>

#include "base/scoped_observation.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_ads/core/browser/service/test/ads_service_waiter.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "brave/components/brave_ads/core/public/prefs/pref_registry.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace brave_ads {

namespace {

class AdsClientNotifierObserverMock : public AdsClientNotifierObserver {
 public:
  MOCK_METHOD(void, OnNotifyPrefDidChange, (const std::string&), (override));
};

}  // namespace

class AdsServiceImplIOSTest : public PlatformTest {
 public:
  AdsServiceImplIOSTest() {
    RegisterProfilePrefs(prefs_.registry());
    ads_service_ = std::make_unique<AdsServiceImplIOS>(prefs_);

    // This stops queueing notifications so they are delivered to
    // observers immediately.
    ads_service_->GetAdsClientNotifier()->NotifyPendingObservers();
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

TEST_F(AdsServiceImplIOSTest,
       NotifiesAdsClientObserversWhenSponsoredAdsPrefChanges) {
  // Arrange
  prefs_.SetBoolean(prefs::kSponsoredEnabled, true);

  AdsClientNotifierObserverMock observer;
  base::ScopedObservation<AdsClientNotifier, AdsClientNotifierObserver>
      observation{&observer};
  observation.Observe(ads_service_->GetAdsClientNotifier());

  // Act & Assert
  EXPECT_CALL(observer,
              OnNotifyPrefDidChange(std::string(prefs::kSponsoredEnabled)));
  prefs_.SetBoolean(prefs::kSponsoredEnabled, false);
}

}  // namespace brave_ads
