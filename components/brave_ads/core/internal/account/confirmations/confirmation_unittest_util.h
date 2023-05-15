/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_UNITTEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_UNITTEST_UTIL_H_

#include "brave/components/brave_ads/core/internal/privacy/tokens/token_generator_interface.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

struct ConfirmationInfo;
struct TransactionInfo;

constexpr char kTransactionId[] = "8b742869-6e4a-490c-ac31-31b49130098a";

constexpr char kGetSignedTokensNonce[] = "2f0e2891-e7a5-4262-835b-550b13e58e5c";

absl::optional<ConfirmationInfo> BuildConfirmation(
    privacy::TokenGeneratorInterface* token_generator,
    const TransactionInfo& transaction);
absl::optional<ConfirmationInfo> BuildConfirmation(
    privacy::TokenGeneratorInterface* token_generator);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_UNITTEST_UTIL_H_
