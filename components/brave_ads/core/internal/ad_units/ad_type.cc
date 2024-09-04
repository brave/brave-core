/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

#include "base/containers/fixed_flat_map.h"
#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

namespace {

// Do not change the following string values as they are used for persisting and
// restoring state.
constexpr char kUndefinedType[] = "";
constexpr char kNotificationAdType[] = "ad_notification";
constexpr char kNewTabPageAdType[] = "new_tab_page_ad";
constexpr char kPromotedContentAdType[] = "promoted_content_ad";
constexpr char kInlineContentAdType[] = "inline_content_ad";
constexpr char kSearchResultAdType[] = "search_result_ad";

constexpr auto kStringToMojomAdTypeMap =
    base::MakeFixedFlatMap<std::string_view, mojom::AdType>(
        {{kUndefinedType, mojom::AdType::kUndefined},
         {kNotificationAdType, mojom::AdType::kNotificationAd},
         {kNewTabPageAdType, mojom::AdType::kNewTabPageAd},
         {kPromotedContentAdType, mojom::AdType::kPromotedContentAd},
         {kInlineContentAdType, mojom::AdType::kInlineContentAd},
         {kSearchResultAdType, mojom::AdType::kSearchResultAd}});

constexpr auto kMojomAdTypeToStringMap =
    base::MakeFixedFlatMap<mojom::AdType, std::string_view>(
        {{mojom::AdType::kUndefined, kUndefinedType},
         {mojom::AdType::kNotificationAd, kNotificationAdType},
         {mojom::AdType::kNewTabPageAd, kNewTabPageAdType},
         {mojom::AdType::kPromotedContentAd, kPromotedContentAdType},
         {mojom::AdType::kInlineContentAd, kInlineContentAdType},
         {mojom::AdType::kSearchResultAd, kSearchResultAdType}});

}  // namespace

mojom::AdType ToMojomAdType(std::string_view value) {
  const auto iter = kStringToMojomAdTypeMap.find(value);
  if (iter != kStringToMojomAdTypeMap.cend()) {
    const auto [_, mojom_ad_type] = *iter;
    return mojom_ad_type;
  }

  NOTREACHED_NORETURN() << "Unexpected value for mojom::AdType: " << value;
}

const char* ToString(mojom::AdType mojom_ad_type) {
  const auto iter = kMojomAdTypeToStringMap.find(mojom_ad_type);
  if (iter != kMojomAdTypeToStringMap.cend()) {
    const auto [_, ad_type] = *iter;
    return ad_type.data();
  }

  NOTREACHED_NORETURN() << "Unexpected value for mojom::AdType: "
                        << base::to_underlying(mojom_ad_type);
}

}  // namespace brave_ads
