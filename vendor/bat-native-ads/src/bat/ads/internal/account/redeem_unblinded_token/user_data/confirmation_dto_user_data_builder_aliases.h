/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_REDEEM_UNBLINDED_TOKEN_USER_DATA_CONFIRMATION_DTO_USER_DATA_BUILDER_ALIASES_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_REDEEM_UNBLINDED_TOKEN_USER_DATA_CONFIRMATION_DTO_USER_DATA_BUILDER_ALIASES_H_

#include <functional>

#include "base/values.h"

namespace ads {
namespace dto {
namespace user_data {

using Callback = std::function<void(const base::Value&)>;

}  // namespace user_data
}  // namespace dto
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_REDEEM_UNBLINDED_TOKEN_USER_DATA_CONFIRMATION_DTO_USER_DATA_BUILDER_ALIASES_H_
