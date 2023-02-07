/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_UPHOLD_UPHOLD_TRANSFER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_UPHOLD_UPHOLD_TRANSFER_H_

#include <string>

#include "bat/ledger/internal/endpoints/uphold/get_transaction_status/get_transaction_status_uphold.h"
#include "bat/ledger/internal/endpoints/uphold/post_commit_transaction/post_commit_transaction_uphold.h"
#include "bat/ledger/internal/endpoints/uphold/post_create_transaction/post_create_transaction_uphold.h"
#include "bat/ledger/internal/wallet_provider/transfer.h"

namespace ledger::uphold {

class UpholdTransfer final : public wallet_provider::Transfer {
 public:
  using Transfer::Transfer;

 private:
  void CreateTransaction(MaybeCreateTransactionCallback,
                         mojom::ExternalTransactionPtr) const override;

  void OnCreateTransaction(
      MaybeCreateTransactionCallback,
      mojom::ExternalTransactionPtr,
      endpoints::PostCreateTransactionUphold::Result&&) const;

  void CommitTransaction(ledger::ResultCallback,
                         mojom::ExternalTransactionPtr) const override;

  void OnCommitTransaction(
      ledger::ResultCallback,
      std::string&& transaction_id,
      endpoints::PostCommitTransactionUphold::Result&&) const;

  void OnGetTransactionStatus(
      ledger::ResultCallback,
      endpoints::GetTransactionStatusUphold::Result&&) const;
};

}  // namespace ledger::uphold

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_UPHOLD_UPHOLD_TRANSFER_H_
