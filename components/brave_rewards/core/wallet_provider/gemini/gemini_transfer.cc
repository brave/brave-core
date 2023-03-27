/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/gemini/gemini_transfer.h"

#include <utility>

#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/gemini/gemini_util.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"

using brave_rewards::core::endpoints::PostCommitTransactionGemini;
using brave_rewards::core::endpoints::RequestFor;

namespace brave_rewards::core::gemini {

void GeminiTransfer::CommitTransaction(
    ResultCallback callback,
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
    ResultCallback callback,
    endpoints::PostCommitTransactionGemini::Result&& result) const {
  if (!ledger_->gemini()->GetWalletIf({mojom::WalletStatus::kConnected})) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  if (!result.has_value()) {
    switch (result.error()) {
      case PostCommitTransactionGemini::Error::kTransactionPending:
        return std::move(callback).Run(
            mojom::Result::RETRY_PENDING_TRANSACTION);
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

}  // namespace brave_rewards::core::gemini
