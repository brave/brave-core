/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_ACTIONS_CONVERSION_ACTION_TYPES_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_ACTIONS_CONVERSION_ACTION_TYPES_UTIL_H_

#include <string>

#include "brave/components/brave_ads/core/internal/conversions/actions/conversion_action_types.h"

namespace brave_ads {

class ConfirmationType;

ConversionActionType ToConversionActionType(
    const ConfirmationType& confirmation_type);

ConversionActionType StringToConversionActionType(
    const std::string& action_type);

std::string ConversionActionTypeToString(ConversionActionType action_type);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CONVERSIONS_ACTIONS_CONVERSION_ACTION_TYPES_UTIL_H_
