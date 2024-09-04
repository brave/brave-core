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

constexpr auto kStringToMojomConfirmationTypeMap =
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
         {kMediaPlayType, mojom::ConfirmationType::kMediaPlay},
         {kMedia25Type, mojom::ConfirmationType::kMedia25},
         {kMedia100Type, mojom::ConfirmationType::kMedia100}});

constexpr auto kMojomConfirmationTypeToStringMap =
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
         {mojom::ConfirmationType::kMediaPlay, kMediaPlayType},
         {mojom::ConfirmationType::kMedia25, kMedia25Type},
         {mojom::ConfirmationType::kMedia100, kMedia100Type}});

}  // namespace

mojom::ConfirmationType ToMojomConfirmationType(const std::string_view value) {
  const auto iter = kStringToMojomConfirmationTypeMap.find(value);
  if (iter != kStringToMojomConfirmationTypeMap.cend()) {
    const auto [_, mojom_confirmation_type] = *iter;
    return mojom_confirmation_type;
  }

  NOTREACHED_NORETURN() << "Unexpected value for mojom::ConfirmationType: "
                        << value;
}

const char* ToString(const mojom::ConfirmationType mojom_confirmation_type) {
  const auto iter =
      kMojomConfirmationTypeToStringMap.find(mojom_confirmation_type);
  if (iter != kMojomConfirmationTypeToStringMap.cend()) {
    const auto [_, confirmation_type] = *iter;
    return confirmation_type.data();
  }

  NOTREACHED_NORETURN() << "Unexpected value for mojom::ConfirmationType: "
                        << base::to_underlying(mojom_confirmation_type);
}

}  // namespace brave_ads
