/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/conversion_user_data_builder.h"

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/conversions/verifiable_conversion_envelope_unittest_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionUserDataBuilderTest : public UnitTestBase {};

TEST_F(BraveAdsConversionUserDataBuilderTest, BuildConversionUserData) {
  // Arrange
  BuildAndSaveConversionQueueItems(AdType::kNotificationAd, kConversionId,
                                   kConversionAdvertiserPublicKey,
                                   /*should_use_random_uuids*/ false,
                                   /*count*/ 1);

  // Act

  // Assert
  BuildVerifiableConversionUserData(
      kCreativeInstanceId, base::BindOnce([](base::Value::Dict user_data) {
        EXPECT_EQ(kConversionId,
                  OpenEnvelopeForUserDataAndAdvertiserSecretKey(
                      user_data, kConversionAdvertiserSecretKey));
      }));
}

TEST_F(BraveAdsConversionUserDataBuilderTest,
       DoNotBuildConversionForMissingCreativeInstanceId) {
  // Arrange
  BuildAndSaveConversionQueueItems(AdType::kNotificationAd, kConversionId,
                                   kConversionAdvertiserPublicKey,
                                   /*should_use_random_uuids*/ false,
                                   /*count*/ 1);

  // Act

  // Assert
  BuildVerifiableConversionUserData(
      kMissingCreativeInstanceId,
      base::BindOnce([](base::Value::Dict user_data) {
        // Assert
        EXPECT_TRUE(user_data.empty());
      }));
}

TEST_F(BraveAdsConversionUserDataBuilderTest,
       DoNotBuildConversionIfConversionIdOrAdvertiserPublicKeyAreEmpty) {
  // Arrange
  BuildAndSaveConversionQueueItems(AdType::kNotificationAd, kEmptyConversionId,
                                   kEmptyConversionAdvertiserPublicKey,
                                   /*should_use_random_uuids*/ false,
                                   /*count*/ 1);

  // Act

  // Assert
  BuildVerifiableConversionUserData(
      kCreativeInstanceId, base::BindOnce([](base::Value::Dict user_data) {
        // Assert
        EXPECT_TRUE(user_data.empty());
      }));
}

}  // namespace brave_ads
