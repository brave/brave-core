/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/wallet_provider/bitflyer/bitflyer_transfer.h"

#include <utility>

#include "brave/components/brave_rewards/core/bitflyer/bitflyer.h"
#include "brave/components/brave_rewards/core/bitflyer/bitflyer_util.h"
#include "brave/components/brave_rewards/core/endpoints/request_for.h"
#include "brave/components/brave_rewards/core/global_constants.h"
#include "brave/components/brave_rewards/core/ledger_impl.h"

using brave_rewards::internal::endpoints::GetTransactionStatusBitFlyer;
using brave_rewards::internal::endpoints::PostCommitTransactionBitFlyer;
using brave_rewards::internal::endpoints::RequestFor;

namespace brave_rewards::internal::bitflyer {

void BitFlyerTransfer::CommitTransaction(
    ResultCallback callback,
    mojom::ExternalTransactionPtr transaction) const {
  if (!transaction) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  DCHECK(!transaction->transaction_id.empty());

  const auto wallet =
      ledger_->bitflyer()->GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  auto on_commit_transaction = base::BindOnce(
      &BitFlyerTransfer::OnCommitTransaction, base::Unretained(this),
      std::move(callback), transaction->transaction_id);

  RequestFor<PostCommitTransactionBitFlyer>(*ledger_, std::move(wallet->token),
                                            std::move(wallet->address),
                                            std::move(transaction))
      .Send(std::move(on_commit_transaction));
}

void BitFlyerTransfer::OnCommitTransaction(
    ResultCallback callback,
    std::string&& transaction_id,
    PostCommitTransactionBitFlyer::Result&& result) const {
  const auto wallet =
      ledger_->bitflyer()->GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  if (!result.has_value()) {
    if (result.error() ==
        PostCommitTransactionBitFlyer::Error::kAccessTokenExpired) {
      if (!ledger_->bitflyer()->LogOutWallet()) {
        BLOG(0, "Failed to disconnect " << constant::kWalletBitflyer
                                        << " wallet!");
      }
    }

    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  RequestFor<GetTransactionStatusBitFlyer>(*ledger_, std::move(wallet->token),
                                           std::move(transaction_id))
      .Send(base::BindOnce(&BitFlyerTransfer::OnGetTransactionStatus,
                           base::Unretained(this), std::move(callback)));
}

void BitFlyerTransfer::OnGetTransactionStatus(
    ResultCallback callback,
    endpoints::GetTransactionStatusBitFlyer::Result&& result) const {
  if (!ledger_->bitflyer()->GetWalletIf({mojom::WalletStatus::kConnected})) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  if (result.has_value()) {
    return std::move(callback).Run(mojom::Result::LEDGER_OK);
  }

  switch (result.error()) {
    case GetTransactionStatusBitFlyer::Error::kTransactionPending:
      return std::move(callback).Run(
          mojom::Result::RETRY_PENDING_TRANSACTION_SHORT);
    case GetTransactionStatusBitFlyer::Error::kAccessTokenExpired:
      if (!ledger_->bitflyer()->LogOutWallet()) {
        BLOG(0, "Failed to disconnect " << constant::kWalletBitflyer
                                        << " wallet!");
      }
      ABSL_FALLTHROUGH_INTENDED;
    default:
      return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }
}

}  // namespace brave_rewards::internal::bitflyer
