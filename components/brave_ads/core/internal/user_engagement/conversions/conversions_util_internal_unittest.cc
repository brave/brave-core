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
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"
#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsConversionsUtilInternalTest : public test::TestBase {};

TEST_F(BraveAdsConversionsUtilInternalTest,
       CanConvertAdEventForNonRewardsUser) {
  // Arrange
  test::DisableBraveRewards();

  // Act & Assert
  for (int i = 0; i < static_cast<int>(AdType::kMaxValue); ++i) {
    const auto ad_type = static_cast<AdType>(i);

    const AdInfo ad = test::BuildAd(ad_type,
                                    /*should_generate_random_uuids=*/false);

    for (int j = 0; j < static_cast<int>(ConfirmationType::kMaxValue); ++j) {
      const auto confirmation_type = static_cast<ConfirmationType>(j);

      bool expected_can_convert_ad_event;
      if (ad_type == AdType::kInlineContentAd ||
          ad_type == AdType::kPromotedContentAd) {
        // For non-Rewards users who have opted into Brave News, allow
        // view-through and click-through conversions.
        expected_can_convert_ad_event =
            confirmation_type == ConfirmationType::kViewedImpression ||
            confirmation_type == ConfirmationType::kClicked;
      } else {
        // Otherwise, only allow click-through conversions.
        expected_can_convert_ad_event =
            confirmation_type == ConfirmationType::kClicked;
      }

      const AdEventInfo ad_event =
          BuildAdEvent(ad, confirmation_type, /*created_at=*/test::Now());
      EXPECT_EQ(expected_can_convert_ad_event, CanConvertAdEvent(ad_event));
    }
  }
}

TEST_F(BraveAdsConversionsUtilInternalTest, CanConvertAdEventForRewardsUser) {
  // Act & Assert
  for (int i = 0; i < static_cast<int>(AdType::kMaxValue); ++i) {
    const auto ad_type = static_cast<AdType>(i);

    const AdInfo ad = test::BuildAd(ad_type,
                                    /*should_generate_random_uuids=*/false);

    for (int j = 0; j < static_cast<int>(ConfirmationType::kMaxValue); ++j) {
      const auto confirmation_type = static_cast<ConfirmationType>(j);

      // Only viewed and clicked ad events are allowed to be converted.
      const bool expected_can_convert_ad_event =
          confirmation_type == ConfirmationType::kViewedImpression ||
          confirmation_type == ConfirmationType::kClicked;

      const AdEventInfo ad_event =
          BuildAdEvent(ad, confirmation_type, /*created_at=*/test::Now());
      EXPECT_EQ(expected_can_convert_ad_event, CanConvertAdEvent(ad_event));
    }
  }
}

}  // namespace brave_ads
