/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ad_units/ad_type.h"

#include "base/check.h"
#include "base/debug/crash_logging.h"
#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace brave_ads {

namespace {

// Do not change the following string values as they are used for persisting and
// restoring state
constexpr char kUndefinedType[] = "";
constexpr char kNotificationAdType[] = "ad_notification";
constexpr char kNewTabPageAdType[] = "new_tab_page_ad";
constexpr char kPromotedContentAdType[] = "promoted_content_ad";
constexpr char kInlineContentAdType[] = "inline_content_ad";
constexpr char kSearchResultAdType[] = "search_result_ad";

}  // namespace

AdType ParseAdType(std::string_view value) {
  if (value == kUndefinedType) {
    return AdType::kUndefined;
  }

  if (value == kNotificationAdType) {
    return AdType::kNotificationAd;
  }

  if (value == kNewTabPageAdType) {
    return AdType::kNewTabPageAd;
  }

  if (value == kPromotedContentAdType) {
    return AdType::kPromotedContentAd;
  }

  if (value == kInlineContentAdType) {
    return AdType::kInlineContentAd;
  }

  if (value == kSearchResultAdType) {
    return AdType::kSearchResultAd;
  }

  SCOPED_CRASH_KEY_STRING32("AdType", "value", value);
  NOTREACHED() << "Unexpected value for AdType: " << value;
  return AdType::kUndefined;
}

AdType FromMojomTypeToAdType(const mojom::AdType value) {
  CHECK(mojom::IsKnownEnumValue(value));

  switch (value) {
    case mojom::AdType::kUndefined: {
      return AdType::kUndefined;
    }

    case mojom::AdType::kNotificationAd: {
      return AdType::kNotificationAd;
    }

    case mojom::AdType::kNewTabPageAd: {
      return AdType::kNewTabPageAd;
    }

    case mojom::AdType::kPromotedContentAd: {
      return AdType::kPromotedContentAd;
    }

    case mojom::AdType::kInlineContentAd: {
      return AdType::kInlineContentAd;
    }

    case mojom::AdType::kSearchResultAd: {
      return AdType::kSearchResultAd;
    }
  }
}

const char* ToString(AdType type) {
  switch (type) {
    case AdType::kUndefined: {
      return kUndefinedType;
    }

    case AdType::kNotificationAd: {
      return kNotificationAdType;
    }

    case AdType::kNewTabPageAd: {
      return kNewTabPageAdType;
    }

    case AdType::kPromotedContentAd: {
      return kPromotedContentAdType;
    }

    case AdType::kInlineContentAd: {
      return kInlineContentAdType;
    }

    case AdType::kSearchResultAd: {
      return kSearchResultAdType;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for AdType: "
                        << base::to_underlying(type);
}

std::ostream& operator<<(std::ostream& os, AdType type) {
  os << ToString(type);
  return os;
}

}  // namespace brave_ads
