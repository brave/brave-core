/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet_provider/gemini/gemini_transfer.h"

#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/endpoints/request_for.h"
#include "bat/ledger/internal/gemini/gemini_util.h"
#include "bat/ledger/internal/ledger_impl.h"

using ledger::endpoints::PostCommitTransactionGemini;
using ledger::endpoints::RequestFor;

namespace ledger::gemini {

void GeminiTransfer::CommitTransaction(
    ledger::ResultCallback callback,
    mojom::ExternalTransactionPtr transaction) const {
  if (!transaction) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  DCHECK(!transaction->transaction_id.empty());

  const auto wallet =
      ledger_->gemini()->GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  auto on_commit_transaction =
      base::BindOnce(&GeminiTransfer::OnCommitTransaction,
                     base::Unretained(this), std::move(callback));

  RequestFor<PostCommitTransactionGemini>(ledger_, std::move(wallet->token),
                                          std::move(wallet->address),
                                          std::move(transaction))
      .Send(std::move(on_commit_transaction));
}

void GeminiTransfer::OnCommitTransaction(
    ledger::ResultCallback callback,
    endpoints::PostCommitTransactionGemini::Result&& result) const {
  if (!ledger_->gemini()->GetWalletIf({mojom::WalletStatus::kConnected})) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  if (!result.has_value()) {
    switch (result.error()) {
      case PostCommitTransactionGemini::Error::kTransactionPending:
        return std::move(callback).Run(mojom::Result::RETRY_LONG);
      case PostCommitTransactionGemini::Error::kAccessTokenExpired:
        if (!ledger_->gemini()->LogOutWallet()) {
          BLOG(0, "Failed to disconnect " << constant::kWalletGemini
                                          << " wallet!");
        }
        ABSL_FALLTHROUGH_INTENDED;
      default:
        return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
    }
  }

  std::move(callback).Run(mojom::Result::LEDGER_OK);
}

}  // namespace ledger::gemini
