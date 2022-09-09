/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/common/random_util.h"
#include "bat/ledger/internal/gemini/gemini_util.h"
#include "bat/ledger/internal/gemini/gemini_wallet.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/logging/event_log_keys.h"
#include "bat/ledger/internal/wallet/wallet_util.h"

using ledger::wallet::OnWalletStatusChange;

namespace ledger {
namespace gemini {

GeminiWallet::GeminiWallet(LedgerImpl* ledger) : ledger_(ledger) {}

GeminiWallet::~GeminiWallet() = default;

void GeminiWallet::Generate(ledger::ResultCallback callback) {
  auto wallet = ledger_->gemini()->GetWallet();
  if (!wallet) {
    wallet = mojom::ExternalWallet::New();
    wallet->type = constant::kWalletGemini;
    wallet->status = mojom::WalletStatus::NOT_CONNECTED;
    if (!ledger_->gemini()->SetWallet(wallet->Clone())) {
      BLOG(0, "Unable to set Gemini wallet!");
      return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    }

    OnWalletStatusChange(ledger_, {}, wallet->status);
  }

  if (wallet->one_time_string.empty()) {
    wallet->one_time_string = util::GenerateRandomHexString();
  }

  absl::optional<mojom::WalletStatus> from;
  if (wallet->token.empty() &&
      (wallet->status == mojom::WalletStatus::PENDING)) {
    from = wallet->status;
    wallet->status = mojom::WalletStatus::NOT_CONNECTED;
  }

  wallet = GenerateLinks(std::move(wallet));
  if (!ledger_->gemini()->SetWallet(wallet->Clone())) {
    BLOG(0, "Unable to set Gemini wallet!");
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  if (from) {
    OnWalletStatusChange(ledger_, from, wallet->status);
  }

  if (wallet->status == mojom::WalletStatus::VERIFIED ||
      wallet->status == mojom::WalletStatus::DISCONNECTED_VERIFIED) {
    return ledger_->promotion()->TransferTokens(base::BindOnce(
        [](ledger::ResultCallback callback, mojom::Result result, std::string) {
          if (result != mojom::Result::LEDGER_OK) {
            BLOG(0, "Claiming tokens failed");
            return std::move(callback).Run(mojom::Result::CONTINUE);
          }
          std::move(callback).Run(mojom::Result::LEDGER_OK);
        },
        std::move(callback)));
  }

  std::move(callback).Run(mojom::Result::LEDGER_OK);
}

}  // namespace gemini
}  // namespace ledger
