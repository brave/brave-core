/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_time_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_unittest_constants.h"
#include "third_party/re2/src/re2/re2.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionUserDataBuilderTest : public UnitTestBase {};

TEST_F(BraveAdsConversionUserDataBuilderTest,
       BuildConversionUserDataForRewardsUser) {
  // Arrange
  const AdInfo ad =
      test::BuildAd(AdType::kNotificationAd, /*should_use_random_uuids=*/false);
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/Now());
  const ConversionInfo conversion =
      BuildConversion(ad_event, /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  const base::Value::Dict expected_user_data = base::test::ParseJsonDict(
      R"(
          {
            "conversion": [
              {
                "action": "view"
              }
            ]
          })");
  EXPECT_EQ(expected_user_data, BuildConversionUserData(conversion));
}

TEST_F(BraveAdsConversionUserDataBuilderTest,
       BuildVerifiableConversionUserDataForRewardsUser) {
  // Arrange
  const AdInfo ad =
      test::BuildAd(AdType::kNotificationAd, /*should_use_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at=*/Now());
  const ConversionInfo conversion = BuildConversion(
      ad_event,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});

  // Act & Assert
  std::string json;
  ASSERT_TRUE(
      base::JSONWriter::Write(BuildConversionUserData(conversion), &json));
  const std::string pattern =
      R"({"conversion":\[{"action":"click"},{"envelope":{"alg":"crypto_box_curve25519xsalsa20poly1305","ciphertext":".{64}","epk":".{44}","nonce":".{32}"}}]})";
  EXPECT_TRUE(RE2::FullMatch(json, pattern));
}

TEST_F(BraveAdsConversionUserDataBuilderTest,
       BuildConversionUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const AdInfo ad =
      test::BuildAd(AdType::kNotificationAd, /*should_use_random_uuids=*/false);
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/Now());
  const ConversionInfo conversion =
      BuildConversion(ad_event, /*verifiable_conversion=*/std::nullopt);

  // Act & Assert
  const base::Value::Dict expected_user_data = base::test::ParseJsonDict(
      R"(
          {
            "conversion": [
              {
                "action": "view"
              }
            ]
          })");
  EXPECT_EQ(expected_user_data, BuildConversionUserData(conversion));
}

TEST_F(BraveAdsConversionUserDataBuilderTest,
       BuildVerifiableConversionUserDataForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  const AdInfo ad =
      test::BuildAd(AdType::kNotificationAd, /*should_use_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at=*/Now());
  const ConversionInfo conversion = BuildConversion(
      ad_event,
      VerifiableConversionInfo{kVerifiableConversionId,
                               kVerifiableConversionAdvertiserPublicKey});

  // Act & Assert
  std::string json;
  ASSERT_TRUE(
      base::JSONWriter::Write(BuildConversionUserData(conversion), &json));
  const std::string pattern =
      R"({"conversion":\[{"action":"click"},{"envelope":{"alg":"crypto_box_curve25519xsalsa20poly1305","ciphertext":".{64}","epk":".{44}","nonce":".{32}"}}]})";
  EXPECT_TRUE(RE2::FullMatch(json, pattern));
}

}  // namespace brave_ads
