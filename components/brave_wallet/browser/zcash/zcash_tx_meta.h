/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TX_META_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TX_META_H_

#include <memory>
#include <utility>

#include "brave/components/brave_wallet/browser/tx_meta.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class ZCashTransaction;

class ZCashTxMeta : public TxMeta {
 public:
  ZCashTxMeta();
  ZCashTxMeta(const mojom::AccountIdPtr& from,
              std::unique_ptr<ZCashTransaction> tx);
  ~ZCashTxMeta() override;
  ZCashTxMeta(const ZCashTxMeta&) = delete;
  ZCashTxMeta operator=(const ZCashTxMeta&) = delete;
  bool operator==(const ZCashTxMeta& other) const;

  // TxMeta
  base::Value::Dict ToValue() const override;
  mojom::TransactionInfoPtr ToTransactionInfo() const override;

  ZCashTransaction* tx() const { return tx_.get(); }
  void set_tx(std::unique_ptr<ZCashTransaction> tx) { tx_ = std::move(tx); }

 private:
  std::unique_ptr<ZCashTransaction> tx_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ZCASH_ZCASH_TX_META_H_
