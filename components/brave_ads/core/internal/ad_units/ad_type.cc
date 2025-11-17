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
constexpr std::string_view kUndefinedType;
constexpr std::string_view kNotificationAdType = "ad_notification";
constexpr std::string_view kNewTabPageAdType = "new_tab_page_ad";
constexpr std::string_view kSearchResultAdType = "search_result_ad";

constexpr auto kStringToMojomMap =
    base::MakeFixedFlatMap<std::string_view, mojom::AdType>(
        {{kUndefinedType, mojom::AdType::kUndefined},
         {kNotificationAdType, mojom::AdType::kNotificationAd},
         {kNewTabPageAdType, mojom::AdType::kNewTabPageAd},
         {kSearchResultAdType, mojom::AdType::kSearchResultAd}});

constexpr auto kMojomToStringMap =
    base::MakeFixedFlatMap<mojom::AdType, std::string_view>(
        {{mojom::AdType::kUndefined, kUndefinedType},
         {mojom::AdType::kNotificationAd, kNotificationAdType},
         {mojom::AdType::kNewTabPageAd, kNewTabPageAdType},
         {mojom::AdType::kSearchResultAd, kSearchResultAdType}});

}  // namespace

mojom::AdType ToMojomAdType(std::string_view value) {
  const auto iter = kStringToMojomMap.find(value);
  if (iter != kStringToMojomMap.cend()) {
    return iter->second;
  }

  NOTREACHED() << "Unexpected value for mojom::AdType: " << value;
}

std::string_view ToString(mojom::AdType value) {
  const auto iter = kMojomToStringMap.find(value);
  if (iter != kMojomToStringMap.cend()) {
    return iter->second;
  }

  NOTREACHED() << "Unexpected value for mojom::AdType: "
               << base::to_underlying(value);
}

}  // namespace brave_ads
