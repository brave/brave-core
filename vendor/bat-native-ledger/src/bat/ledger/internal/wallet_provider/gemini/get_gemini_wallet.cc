/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet_provider/gemini/get_gemini_wallet.h"

#include "bat/ledger/global_constants.h"

using ledger::wallet_provider::GetExternalWallet;

namespace ledger::gemini {

GetGeminiWallet::GetGeminiWallet(LedgerImpl* ledger)
    : GetExternalWallet(ledger) {}

GetGeminiWallet::~GetGeminiWallet() = default;

const char* GetGeminiWallet::WalletType() const {
  return constant::kWalletGemini;
}

}  // namespace ledger::gemini
