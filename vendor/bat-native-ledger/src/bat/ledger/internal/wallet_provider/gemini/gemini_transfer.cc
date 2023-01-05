/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/wallet_provider/gemini/gemini_transfer.h"

#include <utility>

#include "bat/ledger/global_constants.h"
#include "bat/ledger/internal/endpoints/request_for.h"
#include "bat/ledger/internal/gemini/gemini_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "chrome/browser/lifetime/application_lifetime.h"

using ledger::endpoints::PostCommitTransactionGemini;
using ledger::endpoints::RequestFor;

namespace ledger::gemini {

void GeminiTransfer::CommitTransaction(ledger::ResultCallback callback,
                                       std::string&& destination,
                                       double amount,
                                       std::string&& transaction_id) const {
  if (transaction_id.empty()) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  const auto wallet =
      ledger_->gemini()->GetWalletIf({mojom::WalletStatus::kConnected});
  if (!wallet) {
    return std::move(callback).Run(mojom::Result::LEDGER_ERROR);
  }

  auto on_commit_transaction =
      base::BindOnce(&GeminiTransfer::OnCommitTransaction,
                     base::Unretained(this), std::move(callback));

  RequestFor<PostCommitTransactionGemini>(
      ledger_, std::move(wallet->token), std::move(wallet->address),
      std::move(transaction_id), std::move(destination), amount)
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
        return std::move(callback).Run(mojom::Result::RETRY);
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

// void GeminiTransfer::FetchTransactionStatus(
//     const std::string& id,
//     const int attempts,
//     client::TransactionCallback callback) {
//   BLOG(1, "Fetching transaction status " << attempts << " time");
//   auto wallet =
//       ledger_->gemini()->GetWalletIf({mojom::WalletStatus::kConnected});
//   if (!wallet) {
//     BLOG(0, "Unexpected " << constant::kWalletGemini << " wallet status!");
//     return callback(mojom::Result::LEDGER_ERROR, "");
//   }
//
//   auto url_callback = std::bind(&GeminiTransfer::OnTransactionStatus, this,
//   _1,
//                                 id, attempts, callback);
//   gemini_server_->get_transaction()->Request(wallet->token, id,
//   url_callback);
// }
//
// void GeminiTransfer::OnTransactionStatus(const mojom::Result result,
//                                          const std::string& id,
//                                          const int attempts,
//                                          client::TransactionCallback
//                                          callback) {
// }

// void GeminiTransfer::CancelTransaction(const std::string& id,
//                                        client::TransactionCallback callback)
//                                        {
//   auto wallet =
//       ledger_->gemini()->GetWalletIf({mojom::WalletStatus::kConnected});
//   if (!wallet) {
//     BLOG(0, "Unexpected " << constant::kWalletGemini << " wallet status!");
//     return callback(mojom::Result::LEDGER_ERROR, "");
//   }
//
//   gemini_server_->post_cancel_transaction()->Request(
//       wallet->token, id, [id, callback](const mojom::Result result) {
//         BLOG(0, "Gemini transaction id: " << id << " cancelled");
//         callback(mojom::Result::LEDGER_ERROR, "");
//       });
// }

}  // namespace ledger::gemini
