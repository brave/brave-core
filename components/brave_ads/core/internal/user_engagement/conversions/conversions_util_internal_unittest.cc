/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversions_util_internal.h"

#include "brave/components/brave_ads/core/internal/ad_units/ad_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings_test_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_builder.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_info.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionsUtilInternalTest : public test::TestBase {};

TEST_F(BraveAdsConversionsUtilInternalTest,
       CanConvertAdEventForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  for (size_t i = 0; i < static_cast<size_t>(mojom::AdType::kMaxValue); ++i) {
    const auto mojom_ad_type = static_cast<mojom::AdType>(i);

    const AdInfo ad = test::BuildAd(mojom_ad_type,
                                    /*should_generate_random_uuids=*/false);

    for (size_t j = 0;
         j < static_cast<size_t>(mojom::ConfirmationType::kMaxValue); ++j) {
      const auto confirmation_type = static_cast<mojom::ConfirmationType>(j);

      bool expected_can_convert_ad_event;
      if (mojom_ad_type == mojom::AdType::kInlineContentAd ||
          mojom_ad_type == mojom::AdType::kPromotedContentAd) {
        // For non-Rewards users who have opted into Brave News, allow
        // view-through and click-through conversions.
        expected_can_convert_ad_event =
            confirmation_type == mojom::ConfirmationType::kViewedImpression ||
            confirmation_type == mojom::ConfirmationType::kClicked;
      } else {
        // Otherwise, only allow click-through conversions.
        expected_can_convert_ad_event =
            confirmation_type == mojom::ConfirmationType::kClicked;
      }

      const AdEventInfo ad_event =
          BuildAdEvent(ad, confirmation_type, /*created_at=*/test::Now());
      EXPECT_EQ(expected_can_convert_ad_event, CanConvertAdEvent(ad_event));
    }
  }
}

TEST_F(BraveAdsConversionsUtilInternalTest, CanConvertAdEventForRewardsUser) {
  // Act & Assert
  for (size_t i = 0; i < static_cast<size_t>(mojom::AdType::kMaxValue); ++i) {
    const auto mojom_ad_type = static_cast<mojom::AdType>(i);

    const AdInfo ad = test::BuildAd(mojom_ad_type,
                                    /*should_generate_random_uuids=*/false);

    for (size_t j = 0;
         j < static_cast<size_t>(mojom::ConfirmationType::kMaxValue); ++j) {
      const auto confirmation_type = static_cast<mojom::ConfirmationType>(j);

      // Only viewed and clicked ad events are allowed to be converted.
      const bool expected_can_convert_ad_event =
          confirmation_type == mojom::ConfirmationType::kViewedImpression ||
          confirmation_type == mojom::ConfirmationType::kClicked;

      const AdEventInfo ad_event =
          BuildAdEvent(ad, confirmation_type, /*created_at=*/test::Now());
      EXPECT_EQ(expected_can_convert_ad_event, CanConvertAdEvent(ad_event));
    }
  }
}

}  // namespace brave_ads
