/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ads_internals/ads_internals_util.h"

#include <optional>

#include "base/test/test_future.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ad_units/test/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/test/creative_set_conversion_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/test/ad_event_test_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAdsAdsInternalsUtil*

namespace brave_ads {

class BraveAdsAdsInternalsUtilTest : public test::TestBase {};

TEST_F(BraveAdsAdsInternalsUtilTest,
       BuildAdsInternalsContainsCreativeSetConversionJsonKeys) {
  // Arrange
  test::BuildAndSaveCreativeSetConversion(
      /*creative_set_id=*/"creative-set-id",
      /*url_pattern=*/"https://www.brave.com/*",
      /*observation_window=*/base::Days(7));

  // Act
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  BuildAdsInternals(test_future.GetCallback());
  const auto& internals = test_future.Get();

  // Assert
  ASSERT_TRUE(internals);
  const base::ListValue* const conversions =
      internals->FindList("creativeSetConversions");
  ASSERT_TRUE(conversions);
  ASSERT_EQ(1u, conversions->size());
  const base::DictValue* const conversion = (*conversions)[0].GetIfDict();
  ASSERT_TRUE(conversion);
  EXPECT_TRUE(conversion->Find("URL Pattern"));
  EXPECT_TRUE(conversion->Find("Expires At"));
}

TEST_F(BraveAdsAdsInternalsUtilTest, BuildAdsInternalsContainsAdEventJsonKeys) {
  // Arrange
  const AdInfo ad =
      test::BuildAd(mojom::AdType::kNotificationAd, /*use_random_uuids=*/false);
  test::RecordAdEvent(ad, mojom::ConfirmationType::kViewedImpression);

  // Act
  base::test::TestFuture<std::optional<base::DictValue>> test_future;
  BuildAdsInternals(test_future.GetCallback());
  const auto& internals = test_future.Get();

  // Assert
  ASSERT_TRUE(internals);
  const base::ListValue* const ad_events = internals->FindList("adEvents");
  ASSERT_TRUE(ad_events);
  ASSERT_EQ(1u, ad_events->size());
  const base::DictValue* const ad_event = (*ad_events)[0].GetIfDict();
  ASSERT_TRUE(ad_event);
  EXPECT_TRUE(ad_event->Find("Target URL"));
  EXPECT_TRUE(ad_event->Find("Ad Type"));
  EXPECT_TRUE(ad_event->Find("Event Type"));
  EXPECT_TRUE(ad_event->Find("Created At"));
}

}  // namespace brave_ads
