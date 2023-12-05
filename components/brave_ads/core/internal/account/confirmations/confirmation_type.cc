/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

#include "base/debug/crash_logging.h"
#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"

namespace brave_ads {

namespace {

// Do not change the following string values as they are used for persisting and
// restoring state.
constexpr char kUndefinedType[] = "";
constexpr char kClickedType[] = "click";
constexpr char kDismissedType[] = "dismiss";
constexpr char kViewedType[] = "view";
constexpr char kServedType[] = "served";
constexpr char kLandedType[] = "landed";
constexpr char kSavedAdType[] = "bookmark";
constexpr char kMarkAdAsInappropriateType[] = "flag";
constexpr char kLikedAdType[] = "upvote";
constexpr char kDislikedAdType[] = "downvote";
constexpr char kConversionType[] = "conversion";

}  // namespace

ConfirmationType ParseConfirmationType(std::string_view value) {
  if (value == kUndefinedType) {
    return ConfirmationType::kUndefined;
  }

  if (value == kClickedType) {
    return ConfirmationType::kClicked;
  }

  if (value == kDismissedType) {
    return ConfirmationType::kDismissed;
  }

  if (value == kViewedType) {
    return ConfirmationType::kViewed;
  }

  if (value == kServedType) {
    return ConfirmationType::kServed;
  }

  if (value == kLandedType) {
    return ConfirmationType::kLanded;
  }

  if (value == kSavedAdType) {
    return ConfirmationType::kSavedAd;
  }

  if (value == kMarkAdAsInappropriateType) {
    return ConfirmationType::kMarkAdAsInappropriate;
  }

  if (value == kLikedAdType) {
    return ConfirmationType::kLikedAd;
  }

  if (value == kDislikedAdType) {
    return ConfirmationType::kDislikedAd;
  }

  if (value == kConversionType) {
    return ConfirmationType::kConversion;
  }

  SCOPED_CRASH_KEY_STRING32("ConfirmationType", "value", value);
  NOTREACHED() << "Unexpected value for ConfirmationType: " << value;
  return ConfirmationType::kUndefined;
}

const char* ToString(ConfirmationType type) {
  switch (type) {
    case ConfirmationType::kUndefined: {
      return kUndefinedType;
    }

    case ConfirmationType::kClicked: {
      return kClickedType;
    }

    case ConfirmationType::kDismissed: {
      return kDismissedType;
    }

    case ConfirmationType::kViewed: {
      return kViewedType;
    }

    case ConfirmationType::kServed: {
      return kServedType;
    }

    case ConfirmationType::kLanded: {
      return kLandedType;
    }

    case ConfirmationType::kSavedAd: {
      return kSavedAdType;
    }

    case ConfirmationType::kMarkAdAsInappropriate: {
      return kMarkAdAsInappropriateType;
    }

    case ConfirmationType::kLikedAd: {
      return kLikedAdType;
    }

    case ConfirmationType::kDislikedAd: {
      return kDislikedAdType;
    }

    case ConfirmationType::kConversion: {
      return kConversionType;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for ConfirmationType: "
                        << base::to_underlying(type);
}

std::ostream& operator<<(std::ostream& os, ConfirmationType type) {
  os << ToString(type);
  return os;
}

}  // namespace brave_ads
