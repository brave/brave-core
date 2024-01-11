/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/gemini/gemini_transfer.h"

#include <utility>

#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/gemini/gemini.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {

using endpoints::PostCommitTransactionGemini;
using endpoints::RequestFor;

namespace gemini {

void GeminiTransfer::CommitTransaction(
    ResultCallback callback,
    mojom::ExternalTransactionPtr transaction) const {
  if (!transaction) {
    return std::move(callback).Run(mojom::Result::FAILED);
  }

  DCHECK(!transaction->transaction_id.empty());

  const auto wallet =
      engine_->gemini()->GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::FAILED);
  }

  auto on_commit_transaction =
      base::BindOnce(&GeminiTransfer::OnCommitTransaction,
                     base::Unretained(this), std::move(callback));

  RequestFor<PostCommitTransactionGemini>(*engine_, std::move(wallet->token),
                                          std::move(wallet->address),
                                          std::move(transaction))
      .Send(std::move(on_commit_transaction));
}

void GeminiTransfer::OnCommitTransaction(
    ResultCallback callback,
    endpoints::PostCommitTransactionGemini::Result&& result) const {
  if (!engine_->gemini()->GetWalletIf({mojom::WalletStatus::kConnected})) {
    return std::move(callback).Run(mojom::Result::FAILED);
  }

  if (!result.has_value()) {
    switch (result.error()) {
      case PostCommitTransactionGemini::Error::kTransactionPending:
        return std::move(callback).Run(
            mojom::Result::RETRY_PENDING_TRANSACTION_LONG);
      case PostCommitTransactionGemini::Error::kAccessTokenExpired:
        if (!engine_->gemini()->LogOutWallet()) {
          BLOG(0, "Failed to disconnect " << constant::kWalletGemini
                                          << " wallet!");
        }
        ABSL_FALLTHROUGH_INTENDED;
      default:
        return std::move(callback).Run(mojom::Result::FAILED);
    }
  }

  std::move(callback).Run(mojom::Result::OK);
}

}  // namespace gemini

}  // namespace brave_rewards::internal
