/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/conversion_user_data.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/confirmation_type.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/conversions/verifiable_conversion_envelope_info.h"
#include "brave/components/brave_ads/core/internal/conversions/verifiable_conversion_envelope_unittest_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionUserDataTest : public UnitTestBase {};

TEST_F(BraveAdsConversionUserDataTest, GetForConversionConfirmationType) {
  // Arrange
  BuildAndSaveConversionQueueItems(AdType::kNotificationAd, kConversionId,
                                   kConversionAdvertiserPublicKey,
                                   /*should_use_random_uuids*/ false,
                                   /*count*/ 1);

  // Act

  // Assert
  BuildConversionUserData(
      kCreativeInstanceId, ConfirmationType::kConversion,
      base::BindOnce([](base::Value::Dict user_data) {
        ASSERT_TRUE(GetVerifiableConversionEnvelopeForUserData(user_data));
      }));
}

TEST_F(BraveAdsConversionUserDataTest,
       DoNotGetForNonConversionConfirmationType) {
  // Arrange
  BuildAndSaveConversionQueueItems(AdType::kNotificationAd, kConversionId,
                                   kConversionAdvertiserPublicKey,
                                   /*should_use_random_uuids*/ false,
                                   /*count*/ 1);

  // Act

  // Assert
  BuildConversionUserData(
      kCreativeInstanceId, ConfirmationType::kViewed,
      base::BindOnce([](base::Value::Dict user_data) {
        ASSERT_FALSE(GetVerifiableConversionEnvelopeForUserData(user_data));
      }));
}

}  // namespace brave_ads
