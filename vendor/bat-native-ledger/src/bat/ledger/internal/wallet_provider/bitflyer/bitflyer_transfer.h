/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_BITFLYER_BITFLYER_TRANSFER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_BITFLYER_BITFLYER_TRANSFER_H_

#include "bat/ledger/internal/endpoints/bitflyer/post_commit_transaction/post_commit_transaction_bitflyer.h"
#include "bat/ledger/internal/wallet_provider/transfer.h"

namespace ledger::bitflyer {

class BitFlyerTransfer final : public wallet_provider::Transfer {
 public:
  using Transfer::Transfer;

 private:
  void CommitTransaction(ledger::ResultCallback,
                         mojom::ExternalTransactionPtr) const override;

  void OnCommitTransaction(
      ledger::ResultCallback,
      endpoints::PostCommitTransactionBitFlyer::Result&&) const;
};

}  // namespace ledger::bitflyer

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_WALLET_PROVIDER_BITFLYER_BITFLYER_TRANSFER_H_
