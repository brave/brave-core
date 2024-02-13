/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

#include "base/containers/fixed_flat_map.h"
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

constexpr auto kToConfirmationTypeMap =
    base::MakeFixedFlatMap<std::string_view, ConfirmationType>(
        {{kUndefinedType, ConfirmationType::kUndefined},
         {kClickedType, ConfirmationType::kClicked},
         {kDismissedType, ConfirmationType::kDismissed},
         {kViewedType, ConfirmationType::kViewed},
         {kServedType, ConfirmationType::kServed},
         {kLandedType, ConfirmationType::kLanded},
         {kSavedAdType, ConfirmationType::kSavedAd},
         {kMarkAdAsInappropriateType, ConfirmationType::kMarkAdAsInappropriate},
         {kLikedAdType, ConfirmationType::kLikedAd},
         {kDislikedAdType, ConfirmationType::kDislikedAd},
         {kConversionType, ConfirmationType::kConversion}});

constexpr auto kConfirmationTypeToStringMap =
    base::MakeFixedFlatMap<ConfirmationType, std::string_view>(
        {{ConfirmationType::kUndefined, kUndefinedType},
         {ConfirmationType::kClicked, kClickedType},
         {ConfirmationType::kDismissed, kDismissedType},
         {ConfirmationType::kViewed, kViewedType},
         {ConfirmationType::kServed, kServedType},
         {ConfirmationType::kLanded, kLandedType},
         {ConfirmationType::kSavedAd, kSavedAdType},
         {ConfirmationType::kMarkAdAsInappropriate, kMarkAdAsInappropriateType},
         {ConfirmationType::kLikedAd, kLikedAdType},
         {ConfirmationType::kDislikedAd, kDislikedAdType},
         {ConfirmationType::kConversion, kConversionType}});

}  // namespace

ConfirmationType ToConfirmationType(const std::string_view value) {
  const auto* iter = kToConfirmationTypeMap.find(value);
  if (iter != kToConfirmationTypeMap.cend()) {
    return iter->second;
  }

  SCOPED_CRASH_KEY_STRING32("ConfirmationType", "value", value);
  NOTREACHED() << "Unexpected value for ConfirmationType: " << value;
  return ConfirmationType::kUndefined;
}

const char* ToString(const ConfirmationType type) {
  const auto* iter = kConfirmationTypeToStringMap.find(type);
  if (iter != kConfirmationTypeToStringMap.cend()) {
    return iter->second.data();
  }

  NOTREACHED_NORETURN() << "Unexpected value for ConfirmationType: "
                        << base::to_underlying(type);
}

std::ostream& operator<<(std::ostream& os, ConfirmationType type) {
  os << ToString(type);
  return os;
}

}  // namespace brave_ads
