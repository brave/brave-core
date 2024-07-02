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
constexpr char kViewedImpressionType[] = "view";
constexpr char kServedImpressionType[] = "served";
constexpr char kLandedType[] = "landed";
constexpr char kSavedAdType[] = "bookmark";
constexpr char kMarkAdAsInappropriateType[] = "flag";
constexpr char kLikedAdType[] = "upvote";
constexpr char kDislikedAdType[] = "downvote";
constexpr char kConversionType[] = "conversion";
constexpr char kMediaPlayType[] = "media_play";
constexpr char kMedia25Type[] = "media_25";
constexpr char kMedia100Type[] = "media_100";

constexpr auto kToConfirmationTypeMap =
    base::MakeFixedFlatMap<std::string_view, ConfirmationType>(
        {{kUndefinedType, ConfirmationType::kUndefined},
         {kClickedType, ConfirmationType::kClicked},
         {kDismissedType, ConfirmationType::kDismissed},
         {kViewedImpressionType, ConfirmationType::kViewedImpression},
         {kServedImpressionType, ConfirmationType::kServedImpression},
         {kLandedType, ConfirmationType::kLanded},
         {kSavedAdType, ConfirmationType::kSavedAd},
         {kMarkAdAsInappropriateType, ConfirmationType::kMarkAdAsInappropriate},
         {kLikedAdType, ConfirmationType::kLikedAd},
         {kDislikedAdType, ConfirmationType::kDislikedAd},
         {kConversionType, ConfirmationType::kConversion},
         {kMediaPlayType, ConfirmationType::kMediaPlay},
         {kMedia25Type, ConfirmationType::kMedia25},
         {kMedia100Type, ConfirmationType::kMedia100}});

constexpr auto kConfirmationTypeToStringMap =
    base::MakeFixedFlatMap<ConfirmationType, std::string_view>(
        {{ConfirmationType::kUndefined, kUndefinedType},
         {ConfirmationType::kClicked, kClickedType},
         {ConfirmationType::kDismissed, kDismissedType},
         {ConfirmationType::kViewedImpression, kViewedImpressionType},
         {ConfirmationType::kServedImpression, kServedImpressionType},
         {ConfirmationType::kLanded, kLandedType},
         {ConfirmationType::kSavedAd, kSavedAdType},
         {ConfirmationType::kMarkAdAsInappropriate, kMarkAdAsInappropriateType},
         {ConfirmationType::kLikedAd, kLikedAdType},
         {ConfirmationType::kDislikedAd, kDislikedAdType},
         {ConfirmationType::kConversion, kConversionType},
         {ConfirmationType::kMediaPlay, kMediaPlayType},
         {ConfirmationType::kMedia25, kMedia25Type},
         {ConfirmationType::kMedia100, kMedia100Type}});

}  // namespace

ConfirmationType ToConfirmationType(const std::string_view value) {
  const auto iter = kToConfirmationTypeMap.find(value);
  if (iter != kToConfirmationTypeMap.cend()) {
    return iter->second;
  }

  // TODO(https://github.com/brave/brave-browser/issues/32066): Detect
  // potential defects using `NOTREACHED`.
  SCOPED_CRASH_KEY_STRING32("Issue32066", "confirmation_type", value);
  NOTREACHED_IN_MIGRATION()
      << "Unexpected value for ConfirmationType: " << value;
  return ConfirmationType::kUndefined;
}

const char* ToString(const ConfirmationType type) {
  const auto iter = kConfirmationTypeToStringMap.find(type);
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
