// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_ads/ads_client_ios_observer.h"

#include "base/test/bind.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/platform_test.h"

namespace brave_ads {

using BraveAdsClientIOSObserverTest = PlatformTest;

TEST_F(BraveAdsClientIOSObserverTest,
       OnSolveCaptchaToServeAdsRunsCallbackWithGivenIds) {
  // Arrange
  std::string payment_id;
  std::string captcha_id;
  AdsClientIOSObserver observer(base::BindLambdaForTesting(
      [&](const std::string& observed_payment_id,
          const std::string& observed_captcha_id) {
        payment_id = observed_payment_id;
        captcha_id = observed_captcha_id;
      }));

  // Act
  observer.OnSolveCaptchaToServeAds(/*payment_id=*/"payment_id",
                                    /*captcha_id=*/"captcha_id");

  // Assert
  EXPECT_EQ("payment_id", payment_id);
  EXPECT_EQ("captcha_id", captcha_id);
}

}  // namespace brave_ads
