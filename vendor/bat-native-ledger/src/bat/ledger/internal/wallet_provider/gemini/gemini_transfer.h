/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_GEMINI_GEMINI_TRANSFER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_GEMINI_GEMINI_TRANSFER_H_

#include <string>

#include "bat/ledger/internal/endpoints/gemini/post_commit_transaction/post_commit_transaction_gemini.h"
#include "bat/ledger/internal/wallet_provider/transfer.h"

namespace ledger::gemini {

class GeminiTransfer final : public wallet_provider::Transfer {
 public:
  using Transfer::Transfer;

 private:
  void CommitTransaction(ledger::ResultCallback,
                         std::string&& destination,
                         double amount,
                         std::string&& transaction_id) const override;

  void OnCommitTransaction(
      ledger::ResultCallback,
      endpoints::PostCommitTransactionGemini::Result&&) const;

  // void FetchTransactionStatus(const std::string& id,
  //                             const int attempts,
  //                             client::TransactionCallback callback);

  // void OnTransactionStatus(const mojom::Result result,
  //                          const std::string& id,
  //                          const int attempts,
  //                          client::TransactionCallback callback);

  // void CancelTransaction(const std::string& id,
  //                        client::TransactionCallback callback);
};

}  // namespace ledger::gemini

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_GEMINI_GEMINI_TRANSFER_H_
