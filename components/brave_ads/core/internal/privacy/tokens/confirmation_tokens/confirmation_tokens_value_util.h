/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_VALUE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_VALUE_UTIL_H_

#include "brave/components/brave_ads/core/internal/privacy/tokens/confirmation_tokens/confirmation_token_info.h"

#include "base/values.h"

namespace brave_ads::privacy {

base::Value::List ConfirmationTokensToValue(
    const ConfirmationTokenList& confirmation_tokens);

ConfirmationTokenList ConfirmationTokensFromValue(
    const base::Value::List& list);

}  // namespace brave_ads::privacy

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PRIVACY_TOKENS_CONFIRMATION_TOKENS_CONFIRMATION_TOKENS_VALUE_UTIL_H_
