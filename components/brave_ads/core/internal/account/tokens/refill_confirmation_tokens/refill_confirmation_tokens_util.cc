/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/tokens/refill_confirmation_tokens/refill_confirmation_tokens_util.h"

#include "brave/components/brave_ads/core/internal/account/tokens/tokens_feature.h"
#include "brave/components/brave_ads/core/internal/privacy/tokens/confirmation_tokens/confirmation_tokens_util.h"

namespace brave_ads {

bool ShouldRefillConfirmationTokens() {
  return privacy::ConfirmationTokenCount() < kMinConfirmationTokens.Get();
}

int CalculateAmountOfConfirmationTokensToRefill() {
  return kMaxConfirmationTokens.Get() - privacy::ConfirmationTokenCount();
}

}  // namespace brave_ads
