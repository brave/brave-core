/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet_provider/uphold/uphold_transfer.h"

#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/endpoints/request_for.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/uphold/uphold_util.h"
#include "chrome/browser/lifetime/application_lifetime.h"

using ledger::endpoints::GetTransactionStatusUphold;
using ledger::endpoints::PostCommitTransactionUphold;
using ledger::endpoints::PostCreateTransactionUphold;
using ledger::endpoints::RequestFor;

namespace ledger::uphold {

void UpholdTransfer::CreateTransaction(MaybeCreateTransactionCallback callback,
                                       std::string&& destination,
                                       double amount) const {
  const auto wallet =
      ledger_->uphold()->GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run("");
  }

  RequestFor<PostCreateTransactionUphold>(ledger_, std::move(wallet->token),
                                          std::move(wallet->address),
                                          std::move(destination), amount)
      .Send(base::BindOnce(&UpholdTransfer::OnCreateTransaction,
                           base::Unretained(this), std::move(callback)));
}

void UpholdTransfer::OnCreateTransaction(
    MaybeCreateTransactionCallback callback,
    PostCreateTransactionUphold::Result&& result) const {
  if (!ledger_->uphold()->GetWalletIf({mojom::WalletStatus::kConnected})) {
    return std::move(callback).Run("");
  }

  if (!result.has_value()) {
    if (result.error() ==
        PostCreateTransactionUphold::Error::kAccessTokenExpired) {
      if (!ledger_->uphold()->LogOutWallet()) {
        BLOG(0,
             "Failed to disconnect " << constant::kWalletUphold << " wallet!");
      }
    }

    return std::move(callback).Run("");
  }

  std::move(callback).Run(std::move(result.value()));
}

void UpholdTransfer::CommitTransaction(ledger::ResultCallback callback,
                                       std::string&&,
                                       double,
                                       std::string&& transaction_id) const {
  if (transaction_id.empty()) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  const auto wallet =
      ledger_->uphold()->GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  auto on_commit_transaction = base::BindOnce(
      &UpholdTransfer::OnCommitTransaction, base::Unretained(this),
      std::move(callback), transaction_id);

  RequestFor<PostCommitTransactionUphold>(ledger_, std::move(wallet->token),
                                          std::move(wallet->address),
                                          std::move(transaction_id))
      .Send(std::move(on_commit_transaction));
}

void UpholdTransfer::OnCommitTransaction(
    ledger::ResultCallback callback,
    std::string&& transaction_id,
    PostCommitTransactionUphold::Result&& result) const {
  // chrome::AttemptUserExit();

  const auto wallet =
      ledger_->uphold()->GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  if (result.has_value()) {
    return std::move(callback).Run(mojom::Result::LEDGER_OK);
  }

  switch (result.error()) {
    case PostCommitTransactionUphold::Error::kTransactionNotFound:
      break;
    case PostCommitTransactionUphold::Error::kAccessTokenExpired:
      if (!ledger_->uphold()->LogOutWallet()) {
        BLOG(0,
             "Failed to disconnect " << constant::kWalletUphold << " wallet!");
      }
      ABSL_FALLTHROUGH_INTENDED;
    default:
      return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  RequestFor<GetTransactionStatusUphold>(ledger_, std::move(wallet->token),
                                         std::move(transaction_id))
      .Send(base::BindOnce(&UpholdTransfer::OnGetTransactionStatus,
                           base::Unretained(this), std::move(callback)));
}

void UpholdTransfer::OnGetTransactionStatus(
    ledger::ResultCallback callback,
    endpoints::GetTransactionStatusUphold::Result&& result) const {
  if (!ledger_->uphold()->GetWalletIf({mojom::WalletStatus::kConnected})) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  if (!result.has_value()) {
    if (result.error() ==
        GetTransactionStatusUphold::Error::kAccessTokenExpired) {
      if (!ledger_->uphold()->LogOutWallet()) {
        BLOG(0,
             "Failed to disconnect " << constant::kWalletUphold << " wallet!");
      }
    }

    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  std::move(callback).Run(mojom::Result::LEDGER_OK);
}

}  // namespace ledger::uphold
