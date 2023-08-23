/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/conversions/actions/conversion_action_types_util.h"

#include "base/notreached.h"
#include "brave/components/brave_ads/core/internal/conversions/actions/conversion_action_types_constants.h"
#include "brave/components/brave_ads/core/public/confirmation_type.h"

namespace brave_ads {

ConversionActionType ToConversionActionType(
    const ConfirmationType& confirmation_type) {
  switch (confirmation_type.value()) {
    case ConfirmationType::kViewed: {
      return ConversionActionType::kViewThrough;
    }

    case ConfirmationType::kClicked: {
      return ConversionActionType::kClickThrough;
    }

    default: {
      NOTREACHED_NORETURN();
    }
  }
}

ConversionActionType StringToConversionActionType(
    const std::string& action_type) {
  if (action_type == kViewThroughConversionActionType) {
    return ConversionActionType::kViewThrough;
  }

  if (action_type == kClickThroughConversionActionType) {
    return ConversionActionType::kClickThrough;
  }

  NOTREACHED_NORETURN();
}

std::string ConversionActionTypeToString(
    const ConversionActionType action_type) {
  switch (action_type) {
    case ConversionActionType::kViewThrough: {
      return kViewThroughConversionActionType;
    }

    case ConversionActionType::kClickThrough: {
      return kClickThroughConversionActionType;
    }

    default: {
      NOTREACHED_NORETURN();
    }
  }
}

}  // namespace brave_ads
