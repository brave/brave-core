/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/tx_status_resolver.h"

namespace brave_wallet {

TxStatusResolver::TxStatusResolver(TxService* tx_service)
    : tx_service_(tx_service) {}

void TxStatusResolver::GetPendingTransactionsCount(
    GetPendingTransactionsCountCallback callback) {
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&TxStatusResolver::RunCheck,
                                weak_ptr_factory_.GetWeakPtr(),
                                std::move(callback), 0, mojom::CoinType::ETH));
}

void TxStatusResolver::RunCheck(GetPendingTransactionsCountCallback callback,
                                size_t counter,
                                mojom::CoinType coin) {
  tx_service_->GetAllTransactionInfo(
      coin, base::BindOnce(&TxStatusResolver::OnTxResolved,
                           weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                           counter, coin));
}

void TxStatusResolver::OnTxResolved(
    GetPendingTransactionsCountCallback callback,
    size_t counter,
    mojom::CoinType coin,
    std::vector<mojom::TransactionInfoPtr> result) {
  absl::optional<mojom::CoinType> next_coin_to_check;
  counter += GetPendingTransactionsCount(result);

  switch (coin) {
    case mojom::CoinType::ETH:
      next_coin_to_check = mojom::CoinType::FIL;
      break;
    case mojom::CoinType::FIL:
      next_coin_to_check = mojom::CoinType::SOL;
      break;
    case mojom::CoinType::SOL:
      break;
  }
  if (!next_coin_to_check) {
    std::move(callback).Run(counter);
    return;
  }

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&TxStatusResolver::RunCheck,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                     counter, next_coin_to_check.value()));
}

size_t TxStatusResolver::GetPendingTransactionsCount(
    const std::vector<mojom::TransactionInfoPtr>& result) {
  size_t counter = 0u;
  for (const auto& tx : result) {
    if (tx->tx_status == mojom::TransactionStatus::Unapproved) {
      counter++;
    }
  }
  return counter;
}

TxStatusResolver::~TxStatusResolver() {}

}  // namespace brave_wallet
