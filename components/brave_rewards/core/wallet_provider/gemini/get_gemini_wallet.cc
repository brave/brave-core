/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/gemini/get_gemini_wallet.h"

#include "brave/components/brave_rewards/core/global_constants.h"

namespace brave_rewards::internal::gemini {

GetGeminiWallet::GetGeminiWallet(RewardsEngineImpl& engine)
    : wallet_provider::GetExternalWallet(engine) {}

GetGeminiWallet::~GetGeminiWallet() = default;

const char* GetGeminiWallet::WalletType() const {
  return constant::kWalletGemini;
}

}  // namespace brave_rewards::internal::gemini
