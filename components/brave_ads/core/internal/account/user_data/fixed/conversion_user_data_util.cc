/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data_util.h"

#include "base/check_op.h"
#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data_constants.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/actions/conversion_action_types_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"

namespace brave_ads {

base::DictValue BuildConversionActionTypeUserData(
    const ConversionInfo& conversion) {
  CHECK_NE(ConversionActionType::kUndefined, conversion.action_type);

  return base::DictValue().Set(kConversionActionTypeKey,
                               ToString(conversion.action_type));
}

std::optional<base::DictValue> MaybeBuildConversionOrderUserData(
    const ConversionInfo& conversion) {
  if (!conversion.extracted_order.id && !conversion.extracted_order.value) {
    return std::nullopt;
  }

  base::DictValue order;
  if (conversion.extracted_order.id) {
    order.Set(kConversionOrderIdKey, *conversion.extracted_order.id);
  }
  if (conversion.extracted_order.value) {
    order.Set(kConversionOrderValueKey, *conversion.extracted_order.value);
    if (conversion.extracted_order.currency) {
      order.Set(kConversionOrderCurrencyKey,
                *conversion.extracted_order.currency);
    }
  }

  return base::DictValue().Set(kConversionOrderKey, std::move(order));
}

}  // namespace brave_ads
