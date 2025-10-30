/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/new_tab_page_ad/new_tab_page_ad_event_type_util.h"

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

TEST(BraveAdsNewTabPageAdEventTypeUtilTest, ToMojomNewTabPageAdEventType) {
  // Act & Assert
  EXPECT_EQ(ToMojomNewTabPageAdEventType("served"),
            mojom::NewTabPageAdEventType::kServedImpression);

  EXPECT_EQ(ToMojomNewTabPageAdEventType("view"),
            mojom::NewTabPageAdEventType::kViewedImpression);

  EXPECT_EQ(ToMojomNewTabPageAdEventType("click"),
            mojom::NewTabPageAdEventType::kClicked);

  EXPECT_EQ(ToMojomNewTabPageAdEventType("interaction"),
            mojom::NewTabPageAdEventType::kInteraction);

  EXPECT_EQ(ToMojomNewTabPageAdEventType("media_play"),
            mojom::NewTabPageAdEventType::kMediaPlay);

  EXPECT_EQ(ToMojomNewTabPageAdEventType("media_25"),
            mojom::NewTabPageAdEventType::kMedia25);

  EXPECT_EQ(ToMojomNewTabPageAdEventType("media_100"),
            mojom::NewTabPageAdEventType::kMedia100);

  EXPECT_EQ(ToMojomNewTabPageAdEventType(""), std::nullopt);

  EXPECT_EQ(ToMojomNewTabPageAdEventType("foobar"), std::nullopt);
}

TEST(BraveAdsNewTabPageAdEventTypeUtilTest, ToString) {
  // Act & Assert
  EXPECT_EQ(ToString(mojom::NewTabPageAdEventType::kServedImpression),
            "served");

  EXPECT_EQ(ToString(mojom::NewTabPageAdEventType::kViewedImpression), "view");

  EXPECT_EQ(ToString(mojom::NewTabPageAdEventType::kClicked), "click");

  EXPECT_EQ(ToString(mojom::NewTabPageAdEventType::kInteraction),
            "interaction");

  EXPECT_EQ(ToString(mojom::NewTabPageAdEventType::kMediaPlay), "media_play");

  EXPECT_EQ(ToString(mojom::NewTabPageAdEventType::kMedia25), "media_25");

  EXPECT_EQ(ToString(mojom::NewTabPageAdEventType::kMedia100), "media_100");
}

}  // namespace brave_ads
