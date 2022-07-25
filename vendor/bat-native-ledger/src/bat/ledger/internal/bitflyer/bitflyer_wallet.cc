/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/bitflyer/bitflyer_util.h"
#include "bat/ledger/internal/bitflyer/bitflyer_wallet.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/wallet/wallet_util.h"

using ledger::wallet::OnWalletStatusChange;

namespace ledger {
namespace bitflyer {

BitflyerWallet::BitflyerWallet(LedgerImpl* ledger) : ledger_(ledger) {}

BitflyerWallet::~BitflyerWallet() = default;

void BitflyerWallet::Generate(ledger::ResultCallback callback) {
  auto wallet = ledger_->bitflyer()->GetWallet();
  if (!wallet) {
    wallet = type::ExternalWallet::New();
    wallet->type = constant::kWalletBitflyer;
    wallet->status = type::WalletStatus::NOT_CONNECTED;
    if (!ledger_->bitflyer()->SetWallet(wallet->Clone())) {
      BLOG(0, "Unable to set bitFlyer wallet!");
      return std::move(callback).Run(type::Result::LEDGER_ERROR);
    }

    OnWalletStatusChange(ledger_, {}, wallet->status);
  }

  if (wallet->one_time_string.empty()) {
    wallet->one_time_string = util::GenerateRandomHexString();
  }

  if (wallet->code_verifier.empty()) {
    wallet->code_verifier = util::GeneratePKCECodeVerifier();
  }

  absl::optional<type::WalletStatus> from;
  if (wallet->token.empty() &&
      (wallet->status == type::WalletStatus::PENDING ||
       wallet->status == type::WalletStatus::CONNECTED)) {
    from = wallet->status;
    wallet->status = type::WalletStatus::NOT_CONNECTED;
  }

  wallet = GenerateLinks(std::move(wallet));
  if (!ledger_->bitflyer()->SetWallet(wallet->Clone())) {
    BLOG(0, "Unable to set bitFlyer wallet!");
    return std::move(callback).Run(type::Result::LEDGER_ERROR);
  }

  if (from) {
    OnWalletStatusChange(ledger_, from, wallet->status);
  }

  if (wallet->status == type::WalletStatus::VERIFIED ||
      wallet->status == type::WalletStatus::DISCONNECTED_VERIFIED) {
    // If the wallet is verified, attempt to transfer any applicable grants to
    // the user's external wallet.
    //
    // For Uphold, this is accomplished by calling ledger->wallet()->ClaimFunds
    // as the last step of the GenerateWallet flow. ClaimFunds performs both
    // Uphold wallet linking and attempts to drain legacy Brave user funds to
    // that linked wallet. For bitFlyer, wallet linking is performed during
    // authorization, so bypass ClaimFunds and call promotion()->TransferTokens
    // directly.
    return ledger_->promotion()->TransferTokens(base::BindOnce(
        [](ledger::ResultCallback callback, type::Result result, std::string) {
          if (result != type::Result::LEDGER_OK) {
            BLOG(0, "Claiming tokens failed");
            return std::move(callback).Run(type::Result::CONTINUE);
          }
          std::move(callback).Run(type::Result::LEDGER_OK);
        },
        std::move(callback)));
  }

  std::move(callback).Run(type::Result::LEDGER_OK);
}

}  // namespace bitflyer
}  // namespace ledger
