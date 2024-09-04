/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_ACTIONS_CONVERSION_ACTION_TYPES_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_ACTIONS_CONVERSION_ACTION_TYPES_UTIL_H_

#include <string>
#include <string_view>

#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

ConversionActionType ToConversionActionType(
    mojom::ConfirmationType mojom_confirmation_type);

ConversionActionType ToConversionActionType(std::string_view action_type);

std::string ToString(ConversionActionType action_type);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_CONVERSIONS_ACTIONS_CONVERSION_ACTION_TYPES_UTIL_H_
