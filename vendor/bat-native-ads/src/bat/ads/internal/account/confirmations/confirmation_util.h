/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_UTIL_H_

#include <string>

#include "absl/types/optional.h"
#include "base/values.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

namespace privacy {
class TokenGeneratorInterface;
}  // namespace privacy

class AdType;
class ConfirmationType;
struct ConfirmationInfo;

absl::optional<ConfirmationInfo> CreateConfirmation(
    privacy::TokenGeneratorInterface* token_generator,
    base::Time created_at,
    const std::string& transaction_id,
    const std::string& creative_instance_id,
    const ConfirmationType& confirmation_type,
    const AdType& ad_type,
    base::Value::Dict user_data);

bool IsValid(const ConfirmationInfo& confirmation);

void ResetConfirmations();

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_UTIL_H_
