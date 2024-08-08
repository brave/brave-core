/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/types/verifiable_conversion/verifiable_conversion_test_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionUserDataBuilderTest : public test::TestBase {};

TEST_F(BraveAdsConversionUserDataBuilderTest, BuildConversionUserData) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event = BuildAdEvent(
      ad, ConfirmationType::kViewedImpression, /*created_at=*/test::Now());
  const ConversionInfo conversion =
      BuildConversion(ad_event, /*verifiable_conversion=*/std::nullopt);

  // Act
  const base::Value::Dict user_data = BuildConversionUserData(conversion);

  // Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"(
                    {
                      "conversion": [
                        {
                          "action": "view"
                        }
                      ]
                    })"),
            user_data);
}

TEST_F(BraveAdsConversionUserDataBuilderTest,
       BuildVerifiableConversionUserData) {
  // Arrange
  const AdInfo ad = test::BuildAd(AdType::kNotificationAd,
                                  /*should_generate_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, ConfirmationType::kClicked, /*created_at=*/test::Now());
  const ConversionInfo conversion = BuildConversion(
      ad_event, VerifiableConversionInfo{
                    test::kVerifiableConversionId,
                    test::kVerifiableConversionAdvertiserPublicKeyBase64});

  // Act
  const base::Value::Dict user_data = BuildConversionUserData(conversion);

  // Assert
  std::string json;
  ASSERT_TRUE(base::JSONWriter::Write(user_data, &json));

  EXPECT_THAT(
      json,
      ::testing::MatchesRegex(
          R"({"conversion":\[{"action":"click"},{"envelope":{"alg":"crypto_box_curve25519xsalsa20poly1305","ciphertext":".{64}","epk":".{44}","nonce":".{32}"}}]})"));
}

}  // namespace brave_ads
