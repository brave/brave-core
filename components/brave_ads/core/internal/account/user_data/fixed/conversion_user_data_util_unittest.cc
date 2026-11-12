/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data_util.h"

#include "base/test/values_test_util.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

// pnpm test brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionUserDataUtilTest : public test::TestBase {};

TEST_F(BraveAdsConversionUserDataUtilTest, BuildConversionActionTypeUserData) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  const ConversionInfo conversion = BuildConversion(ad_event);

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "action": "view"
                    })JSON"),
            BuildConversionActionTypeUserData(conversion));
}

TEST_F(BraveAdsConversionUserDataUtilTest,
       BuildConversionOrderUserDataWithIdValueAndCurrency) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  ConversionInfo conversion = BuildConversion(ad_event);
  conversion.order_id = "336121537";
  conversion.order_value = "4,98";
  conversion.order_currency = "EUR";

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "orderId": "336121537",
                      "orderValue": "4,98",
                      "orderCurrency": "EUR"
                    })JSON"),
            MaybeBuildConversionOrderUserData(conversion));
}

// A value selector can resolve to a bare total with no currency marker
// nearby, leaving `order_currency` unset despite a found `order_value`.
TEST_F(BraveAdsConversionUserDataUtilTest,
       BuildConversionOrderUserDataWithValueButNoCurrency) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  ConversionInfo conversion = BuildConversion(ad_event);
  conversion.order_value = "42,42";

  // Act & Assert
  EXPECT_EQ(base::test::ParseJsonDict(
                R"JSON(
                    {
                      "orderValue": "42,42"
                    })JSON"),
            MaybeBuildConversionOrderUserData(conversion));
}

TEST_F(BraveAdsConversionUserDataUtilTest,
       DoNotBuildConversionOrderUserDataIfNeitherIdNorValueSet) {
  // Arrange
  const AdInfo ad = test::BuildAd(mojom::AdType::kNotificationAd,
                                  /*use_random_uuids=*/false);
  const AdEventInfo ad_event =
      BuildAdEvent(ad, mojom::ConfirmationType::kViewedImpression,
                   /*created_at=*/test::Now());
  const ConversionInfo conversion = BuildConversion(ad_event);

  // Act & Assert
  EXPECT_FALSE(MaybeBuildConversionOrderUserData(conversion));
}

}  // namespace brave_ads
