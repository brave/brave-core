/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

#include "base/containers/fixed_flat_map.h"
#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

namespace {

// Do not change the following string values as they are used for persisting and
// restoring state.
constexpr std::string_view kUndefinedType;
constexpr std::string_view kClickedType = "click";
constexpr std::string_view kDismissedType = "dismiss";
constexpr std::string_view kViewedImpressionType = "view";
constexpr std::string_view kServedImpressionType = "served";
constexpr std::string_view kLandedType = "landed";
constexpr std::string_view kSavedAdType = "bookmark";
constexpr std::string_view kMarkAdAsInappropriateType = "flag";
constexpr std::string_view kLikedAdType = "upvote";
constexpr std::string_view kDislikedAdType = "downvote";
constexpr std::string_view kConversionType = "conversion";
constexpr std::string_view kInteractionType = "interaction";
constexpr std::string_view kMediaPlayType = "media_play";
constexpr std::string_view kMedia25Type = "media_25";
constexpr std::string_view kMedia100Type = "media_100";

constexpr auto kStringToMojomMap =
    base::MakeFixedFlatMap<std::string_view, mojom::ConfirmationType>(
        {{kUndefinedType, mojom::ConfirmationType::kUndefined},
         {kClickedType, mojom::ConfirmationType::kClicked},
         {kDismissedType, mojom::ConfirmationType::kDismissed},
         {kViewedImpressionType, mojom::ConfirmationType::kViewedImpression},
         {kServedImpressionType, mojom::ConfirmationType::kServedImpression},
         {kLandedType, mojom::ConfirmationType::kLanded},
         {kSavedAdType, mojom::ConfirmationType::kSavedAd},
         {kMarkAdAsInappropriateType,
          mojom::ConfirmationType::kMarkAdAsInappropriate},
         {kLikedAdType, mojom::ConfirmationType::kLikedAd},
         {kDislikedAdType, mojom::ConfirmationType::kDislikedAd},
         {kConversionType, mojom::ConfirmationType::kConversion},
         {kInteractionType, mojom::ConfirmationType::kInteraction},
         {kMediaPlayType, mojom::ConfirmationType::kMediaPlay},
         {kMedia25Type, mojom::ConfirmationType::kMedia25},
         {kMedia100Type, mojom::ConfirmationType::kMedia100}});

constexpr auto kMojomToStringMap =
    base::MakeFixedFlatMap<mojom::ConfirmationType, std::string_view>(
        {{mojom::ConfirmationType::kUndefined, kUndefinedType},
         {mojom::ConfirmationType::kClicked, kClickedType},
         {mojom::ConfirmationType::kDismissed, kDismissedType},
         {mojom::ConfirmationType::kViewedImpression, kViewedImpressionType},
         {mojom::ConfirmationType::kServedImpression, kServedImpressionType},
         {mojom::ConfirmationType::kLanded, kLandedType},
         {mojom::ConfirmationType::kSavedAd, kSavedAdType},
         {mojom::ConfirmationType::kMarkAdAsInappropriate,
          kMarkAdAsInappropriateType},
         {mojom::ConfirmationType::kLikedAd, kLikedAdType},
         {mojom::ConfirmationType::kDislikedAd, kDislikedAdType},
         {mojom::ConfirmationType::kConversion, kConversionType},
         {mojom::ConfirmationType::kInteraction, kInteractionType},
         {mojom::ConfirmationType::kMediaPlay, kMediaPlayType},
         {mojom::ConfirmationType::kMedia25, kMedia25Type},
         {mojom::ConfirmationType::kMedia100, kMedia100Type}});

}  // namespace

mojom::ConfirmationType ToMojomConfirmationType(std::string_view value) {
  const auto iter = kStringToMojomMap.find(value);
  if (iter != kStringToMojomMap.cend()) {
    return iter->second;
  }

  NOTREACHED() << "Unexpected value for mojom::ConfirmationType: " << value;
}

std::string_view ToString(mojom::ConfirmationType value) {
  const auto iter = kMojomToStringMap.find(value);
  if (iter != kMojomToStringMap.cend()) {
    return iter->second;
  }

  NOTREACHED() << "Unexpected value for mojom::ConfirmationType: "
               << base::to_underlying(value);
}

}  // namespace brave_ads
