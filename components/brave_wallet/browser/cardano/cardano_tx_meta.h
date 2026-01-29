/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TX_META_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TX_META_H_

#include <memory>
#include <utility>

#include "brave/components/brave_wallet/browser/cardano/cardano_transaction.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class CardanoTxMeta : public TxMeta {
 public:
  CardanoTxMeta();
  CardanoTxMeta(const mojom::AccountIdPtr& from,
                std::unique_ptr<CardanoTransaction> tx);
  ~CardanoTxMeta() override;
  CardanoTxMeta(const CardanoTxMeta&) = delete;
  CardanoTxMeta operator=(const CardanoTxMeta&) = delete;
  bool operator==(const CardanoTxMeta& other) const;

  // TxMeta
  base::Value::Dict ToValue() const override;
  mojom::TransactionInfoPtr ToTransactionInfo() const override;
  mojom::CoinType GetCoinType() const override;

  CardanoTransaction* tx() const { return tx_.get(); }
  void set_tx(std::unique_ptr<CardanoTransaction> tx) { tx_ = std::move(tx); }

 private:
  std::unique_ptr<CardanoTransaction> tx_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_TX_META_H_
