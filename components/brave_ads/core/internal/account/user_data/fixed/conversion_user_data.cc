/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data.h"


#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data_constants.h"
#include "brave/components/brave_ads/core/internal/account/user_data/fixed/conversion_user_data_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/conversions/conversion/conversion_info.h"

namespace brave_ads {

base::DictValue BuildConversionUserData(const ConversionInfo& conversion) {
  base::ListValue list;

  // Conversion.
  list.Append(BuildConversionActionTypeUserData(conversion));

  return base::DictValue().Set(kConversionKey, std::move(list));
}

}  // namespace brave_ads
