/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

#include "base/check.h"
#include "base/containers/fixed_flat_map.h"
#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"

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

constexpr auto kToAdTypeMap = base::MakeFixedFlatMap<std::string_view, AdType>(
    {{kUndefinedType, AdType::kUndefined},
     {kNotificationAdType, AdType::kNotificationAd},
     {kNewTabPageAdType, AdType::kNewTabPageAd},
     {kPromotedContentAdType, AdType::kPromotedContentAd},
     {kInlineContentAdType, AdType::kInlineContentAd},
     {kSearchResultAdType, AdType::kSearchResultAd}});

constexpr auto kAdTypeToStringMap =
    base::MakeFixedFlatMap<AdType, std::string_view>(
        {{AdType::kUndefined, kUndefinedType},
         {AdType::kNotificationAd, kNotificationAdType},
         {AdType::kNewTabPageAd, kNewTabPageAdType},
         {AdType::kPromotedContentAd, kPromotedContentAdType},
         {AdType::kInlineContentAd, kInlineContentAdType},
         {AdType::kSearchResultAd, kSearchResultAdType}});

}  // namespace

AdType ToAdType(std::string_view value) {
  const auto iter = kToAdTypeMap.find(value);
  if (iter != kToAdTypeMap.cend()) {
    const auto [_, ad_type] = *iter;
    return ad_type;
  }

  NOTREACHED_NORETURN() << "Unexpected value for AdType: " << value;
}

const char* ToString(AdType type) {
  const auto iter = kAdTypeToStringMap.find(type);
  if (iter != kAdTypeToStringMap.cend()) {
    const auto [_, ad_type] = *iter;
    return ad_type.data();
  }

  NOTREACHED_NORETURN() << "Unexpected value for AdType: "
                        << base::to_underlying(type);
}

std::ostream& operator<<(std::ostream& os, AdType type) {
  os << ToString(type);
  return os;
}

}  // namespace brave_ads
