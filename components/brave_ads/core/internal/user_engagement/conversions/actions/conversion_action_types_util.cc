/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types_util.h"

#include "base/notreached.h"
#include "base/types/cxx23_to_underlying.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types_constants.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

ConversionActionType ToConversionActionType(
    const mojom::ConfirmationType mojom_confirmation_type) {
  switch (mojom_confirmation_type) {
    case mojom::ConfirmationType::kViewedImpression: {
      return ConversionActionType::kViewThrough;
    }

    case mojom::ConfirmationType::kClicked: {
      return ConversionActionType::kClickThrough;
    }

    default: {
      break;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for mojom::ConfirmationType: "
                        << base::to_underlying(mojom_confirmation_type);
}

ConversionActionType ToConversionActionType(
    const std::string_view action_type) {
  if (action_type == kViewThroughConversionActionType) {
    return ConversionActionType::kViewThrough;
  }

  if (action_type == kClickThroughConversionActionType) {
    return ConversionActionType::kClickThrough;
  }

  NOTREACHED_NORETURN() << "Unexpected value for action_type: " << action_type;
}

std::string ToString(const ConversionActionType action_type) {
  switch (action_type) {
    case ConversionActionType::kViewThrough: {
      return kViewThroughConversionActionType;
    }

    case ConversionActionType::kClickThrough: {
      return kClickThroughConversionActionType;
    }

    default: {
      break;
    }
  }

  NOTREACHED_NORETURN() << "Unexpected value for ConversionActionType: "
                        << base::to_underlying(action_type);
}

}  // namespace brave_ads
