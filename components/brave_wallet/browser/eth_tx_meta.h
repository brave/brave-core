/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_META_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_META_H_

#include <memory>
#include <utility>

#include "brave/components/brave_wallet/browser/eth_transaction.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

struct TransactionReceipt;

class EthTxMeta : public TxMeta {
 public:
  EthTxMeta();
  explicit EthTxMeta(std::unique_ptr<EthTransaction> tx);
  ~EthTxMeta() override;
  EthTxMeta(const EthTxMeta&) = delete;
  EthTxMeta operator=(const EthTxMeta&) = delete;
  bool operator==(const EthTxMeta&) const;

  // TxMeta
  base::Value ToValue() const override;
  mojom::TransactionInfoPtr ToTransactionInfo() const override;

  TransactionReceipt tx_receipt() const { return tx_receipt_; }
  EthTransaction* tx() const { return tx_.get(); }

  void set_tx_receipt(const TransactionReceipt& tx_receipt) {
    tx_receipt_ = tx_receipt;
  }
  void set_tx(std::unique_ptr<EthTransaction> tx) { tx_ = std::move(tx); }

 private:
  TransactionReceipt tx_receipt_;
  std::unique_ptr<EthTransaction> tx_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_TX_META_H_
