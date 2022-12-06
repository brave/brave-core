/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_STATUS_RESOLVER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_STATUS_RESOLVER_H_

#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/tx_service.h"

namespace brave_wallet {

using GetPendingTransactionsCountCallback = base::OnceCallback<void(size_t)>;

class TxStatusResolver {
 public:
  explicit TxStatusResolver(TxService* tx_service);
  ~TxStatusResolver();
  void GetPendingTransactionsCount(
      GetPendingTransactionsCountCallback callback);

 private:
  void RunCheck(GetPendingTransactionsCountCallback callback,
                size_t counter,
                mojom::CoinType coin);
  size_t GetPendingTransactionsCount(
      const std::vector<mojom::TransactionInfoPtr>& result);

  void OnTxResolved(GetPendingTransactionsCountCallback callback,
                    size_t counter,
                    mojom::CoinType coin,
                    std::vector<mojom::TransactionInfoPtr> result);

  raw_ptr<TxService> tx_service_;
  base::WeakPtrFactory<TxStatusResolver> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_TX_STATUS_RESOLVER_H_
