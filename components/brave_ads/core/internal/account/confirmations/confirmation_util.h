/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_UTIL_H_

#include <string>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace privacy {
class TokenGeneratorInterface;
}  // namespace privacy

struct ConfirmationInfo;
struct OptedInUserDataInfo;
struct TransactionInfo;

absl::optional<std::string> CreateOptedInCredential(
    const ConfirmationInfo& confirmation);
absl::optional<ConfirmationInfo> CreateOptedInConfirmation(
    privacy::TokenGeneratorInterface* token_generator,
    const TransactionInfo& transaction,
    const OptedInUserDataInfo& opted_in_user_data);

absl::optional<ConfirmationInfo> CreateOptedOutConfirmation(
    const TransactionInfo& transaction);

bool IsValid(const ConfirmationInfo& confirmation);

void ResetConfirmations();

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATION_UTIL_H_
