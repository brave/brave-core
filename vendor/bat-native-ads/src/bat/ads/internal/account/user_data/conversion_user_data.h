/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_CONVERSION_USER_DATA_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_CONVERSION_USER_DATA_H_

#include <string>

#include "bat/ads/internal/account/user_data/conversion_user_data_aliases.h"

namespace ads {

class ConfirmationType;

namespace user_data {

void GetConversion(const std::string& creative_instance_id,
                   const ConfirmationType& confirmation_type,
                   ConversionCallback callback);

}  // namespace user_data
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_USER_DATA_CONVERSION_USER_DATA_H_
