/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/ad_type.h"

#include "base/check.h"
#include "base/notreached.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-shared.h"

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

AdType::AdType() = default;

AdType::AdType(const std::string& value) {
  if (value == kUndefinedType) {
    value_ = kUndefined;
  } else if (value == kNotificationAdType) {
    value_ = kNotificationAd;
  } else if (value == kNewTabPageAdType) {
    value_ = kNewTabPageAd;
  } else if (value == kPromotedContentAdType) {
    value_ = kPromotedContentAd;
  } else if (value == kInlineContentAdType) {
    value_ = kInlineContentAd;
  } else if (value == kSearchResultAdType) {
    value_ = kSearchResultAd;
  } else {
    NOTREACHED_NORETURN();
  }
}

AdType::AdType(const mojom::AdType value) {
  CHECK(mojom::IsKnownEnumValue(value));

  switch (value) {
    case mojom::AdType::kUndefined: {
      value_ = kUndefined;
      break;
    }

    case mojom::AdType::kNotificationAd: {
      value_ = kNotificationAd;
      break;
    }

    case mojom::AdType::kNewTabPageAd: {
      value_ = kNewTabPageAd;
      break;
    }

    case mojom::AdType::kPromotedContentAd: {
      value_ = kPromotedContentAd;
      break;
    }

    case mojom::AdType::kInlineContentAd: {
      value_ = kInlineContentAd;
      break;
    }

    case mojom::AdType::kSearchResultAd: {
      value_ = kSearchResultAd;
      break;
    }
  }
}

AdType::Value AdType::value() const {
  return value_;
}

std::string AdType::ToString() const {
  switch (value_) {
    case kUndefined: {
      return kUndefinedType;
    }

    case kNotificationAd: {
      return kNotificationAdType;
    }

    case kNewTabPageAd: {
      return kNewTabPageAdType;
    }

    case kPromotedContentAd: {
      return kPromotedContentAdType;
    }

    case kInlineContentAd: {
      return kInlineContentAdType;
    }

    case kSearchResultAd: {
      return kSearchResultAdType;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for Value: " << value_;
}

bool operator==(const AdType& lhs, const AdType& rhs) {
  return lhs.value() == rhs.value();
}

bool operator!=(const AdType& lhs, const AdType& rhs) {
  return !(lhs == rhs);
}

std::ostream& operator<<(std::ostream& os, const AdType& type) {
  os << type.ToString();
  return os;
}

}  // namespace brave_ads
