/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_TX_META_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_TX_META_H_

#include <memory>
#include <utility>

#include "brave/components/brave_wallet/browser/fil_transaction.h"
#include "brave/components/brave_wallet/browser/tx_meta.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

namespace base {
class Value;
}  // namespace base

namespace brave_wallet {

class FilTransaction;

class FilTxMeta : public TxMeta {
 public:
  FilTxMeta();
  explicit FilTxMeta(std::unique_ptr<FilTransaction> tx);
  FilTxMeta(const FilTxMeta&) = delete;
  ~FilTxMeta() override;
  bool operator==(const FilTxMeta&) const;

  // TxMeta
  base::Value ToValue() const override;
  mojom::TransactionInfoPtr ToTransactionInfo() const override;

  FilTransaction* tx() const { return tx_.get(); }
  void set_tx(std::unique_ptr<FilTransaction> tx) { tx_ = std::move(tx); }

 private:
  std::unique_ptr<FilTransaction> tx_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_FIL_TX_META_H_
