/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_META_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_META_H_

#include <memory>
#include <utility>

#include "brave/components/brave_wallet/browser/solana_transaction.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class SolanaTransaction;

class SolanaTxMeta : public TxMeta {
 public:
  SolanaTxMeta();
  SolanaTxMeta(const mojom::AccountIdPtr& from,
               std::unique_ptr<SolanaTransaction> tx);
  SolanaTxMeta(const SolanaTxMeta&) = delete;
  ~SolanaTxMeta() override;
  bool operator==(const SolanaTxMeta&) const;

  // TxMeta
  base::Value::Dict ToValue() const override;
  mojom::TransactionInfoPtr ToTransactionInfo() const override;
  mojom::CoinType GetCoinType() const override;

  SolanaTransaction* tx() const { return tx_.get(); }
  SolanaSignatureStatus signature_status() const { return signature_status_; }

  void set_tx(std::unique_ptr<SolanaTransaction> tx) { tx_ = std::move(tx); }
  void set_signature_status(const SolanaSignatureStatus& signature_status) {
    signature_status_ = signature_status;
  }

  bool IsRetriable() const;

 private:
  std::unique_ptr<SolanaTransaction> tx_;
  // Status returned by getSignatureStatuses JSON-RPC call.
  SolanaSignatureStatus signature_status_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_TX_META_H_
