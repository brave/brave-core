/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_type.h"

#include "base/notreached.h"

namespace ads {

namespace {

// Do not change the following string values as they are used for persisting and
// restoring state
const char kUndefinedType[] = "";
const char kAdNotificationType[] = "ad_notification";
const char kNewTabPageAdType[] = "new_tab_page_ad";
const char kPromotedContentAdType[] = "promoted_content_ad";
const char kInlineContentAdType[] = "inline_content_ad";

}  // namespace

AdType::AdType(const std::string& value) {
  if (value == kUndefinedType) {
    value_ = kUndefined;
  } else if (value == kAdNotificationType) {
    value_ = kAdNotification;
  } else if (value == kNewTabPageAdType) {
    value_ = kNewTabPageAd;
  } else if (value == kPromotedContentAdType) {
    value_ = kPromotedContentAd;
  } else if (value == kInlineContentAdType) {
    value_ = kInlineContentAd;
  } else {
    NOTREACHED();
  }
}

AdType::AdType(const mojom::AdType value) {
  switch (value) {
    case mojom::AdType::kUndefined: {
      value_ = kUndefined;
      break;
    }

    case mojom::AdType::kAdNotification: {
      value_ = kAdNotification;
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
  }
}

AdType::Value AdType::value() const {
  return value_;
}

AdType::operator std::string() const {
  return ToString();
}

std::string AdType::ToString() const {
  switch (value_) {
    case kUndefined: {
      return kUndefinedType;
    }

    case kAdNotification: {
      return kAdNotificationType;
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
  }
}

bool AdType::operator==(const AdType& rhs) const {
  return value_ == rhs.value_;
}

bool AdType::operator!=(const AdType& rhs) const {
  return value_ != rhs.value_;
}

}  // namespace ads
