/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_PAYMENT_TOKENS_JSON_READER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_PAYMENT_TOKENS_JSON_READER_H_

#include <string>

#include "brave/components/brave_ads/core/internal/privacy/tokens/payment_tokens/payment_token_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads::rewards::json::reader {

absl::optional<privacy::PaymentTokenList> ReadPaymentTokens(
    const std::string& json);

}  // namespace brave_ads::rewards::json::reader

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_LEGACY_MIGRATION_REWARDS_LEGACY_REWARDS_MIGRATION_PAYMENT_TOKENS_JSON_READER_H_
