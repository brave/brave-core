/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/conversion_user_data.h"

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_constants.h"
#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/conversions/queue/queue_item/conversion_queue_item_unittest_util.h"
#include "brave/components/brave_ads/core/internal/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "third_party/re2/src/re2/re2.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionUserDataBuilderTest : public UnitTestBase {};

TEST_F(BraveAdsConversionUserDataBuilderTest, BuildConversionUserData) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_uuids*/ true);
  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItems(conversion, /*count*/ 1);
  SaveConversionQueueItems(conversion_queue_items);

  // Act

  // Assert
  BuildConversionUserData(
      conversion.creative_instance_id,
      base::BindOnce([](base::Value::Dict user_data) {
        std::string json;
        ASSERT_TRUE(base::JSONWriter::Write(user_data, &json));

        const std::string pattern =
            R"~({"conversion":\[{"action":"view"},{"envelope":{"alg":"crypto_box_curve25519xsalsa20poly1305","ciphertext":"(.{64})","epk":"(.{44})","nonce":"(.{32})"}}]})~";
        EXPECT_TRUE(RE2::FullMatch(json, pattern));
      }));
}

TEST_F(BraveAdsConversionUserDataBuilderTest,
       DoNotBuildConversionForMissingCreativeInstanceId) {
  // Arrange
  const AdInfo ad =
      BuildAd(AdType::kNotificationAd, /*should_use_random_uuids*/ true);
  const ConversionInfo conversion = BuildConversion(
      BuildAdEvent(ad, ConfirmationType::kViewed, /*created_at*/ Now()),
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});
  const ConversionQueueItemList conversion_queue_items =
      BuildConversionQueueItems(conversion, /*count*/ 1);
  SaveConversionQueueItems(conversion_queue_items);

  // Act

  // Assert
  BuildConversionUserData(kMissingCreativeInstanceId,
                          base::BindOnce([](base::Value::Dict user_data) {
                            // Assert
                            EXPECT_TRUE(user_data.empty());
                          }));
}

TEST_F(BraveAdsConversionUserDataBuilderTest,
       DoNotBuildConversionIfConversionQueueIsEmpty) {
  // Arrange

  // Act

  // Assert
  BuildConversionUserData(kCreativeInstanceId,
                          base::BindOnce([](base::Value::Dict user_data) {
                            // Assert
                            EXPECT_TRUE(user_data.empty());
                          }));
}

}  // namespace brave_ads
