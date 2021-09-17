/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_USER_DATA_CONFIRMATION_DTO_USER_DATA_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_USER_DATA_CONFIRMATION_DTO_USER_DATA_BUILDER_H_

#include <string>

#include "bat/ads/internal/tokens/redeem_unblinded_token/user_data/confirmation_dto_user_data_builder_aliases.h"

namespace ads {

class ConfirmationType;

namespace dto {
namespace user_data {

void Build(const std::string& creative_instance_id,
           const ConfirmationType& confirmation_type,
           Callback callback);

}  // namespace user_data
}  // namespace dto
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_USER_DATA_CONFIRMATION_DTO_USER_DATA_BUILDER_H_
