/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/ad_type.h"

#include "bat/ads/internal/logging.h"

namespace ads {

namespace {

// Do not change the following string values as they are used for persisting and
// restoring state
const char kUndefinedType[] = "";
const char kAdNotificationType[] = "ad_notification";
const char kNewTabPageAdType[] = "new_tab_page_ad";

}  // namespace

AdType::AdType(
    const std::string& value) {
  if (value == kUndefinedType) {
    value_ = kUndefined;
  } else if (value == kAdNotificationType) {
    value_ = kAdNotification;
  } else if (value == kNewTabPageAdType) {
    value_ = kNewTabPageAd;
  } else {
    NOTREACHED();
  }
}

AdType::Value AdType::value() const {
  return value_;
}

AdType::operator std::string() const {
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
  }
}

bool AdType::operator==(
    const AdType& rhs) const {
  return value_ == rhs.value_;
}

bool AdType::operator!=(
    const AdType& rhs) const {
  return value_ != rhs.value_;
}

}  // namespace ads
