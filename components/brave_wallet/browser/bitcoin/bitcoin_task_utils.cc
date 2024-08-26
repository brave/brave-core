/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_task_utils.h"

#include <stdint.h>

#include "base/strings/string_number_conversions.h"

namespace brave_wallet {

namespace {

struct ChainBalance {
  uint64_t funded = 0;
  uint64_t spent = 0;
};

ChainBalance GetChainBalance(
    const bitcoin_rpc::AddressChainStats& chain_stats) {
  ChainBalance result;
  if (!base::StringToUint64(chain_stats.funded_txo_sum, &result.funded)) {
    return {};
  }
  if (!base::StringToUint64(chain_stats.spent_txo_sum, &result.spent)) {
    return {};
  }

  return result;
}

}  // namespace

void UpdateBalance(mojom::BitcoinBalancePtr& balance,
                   const bitcoin_rpc::AddressStats& address_stats) {
  auto chain_balance = GetChainBalance(address_stats.chain_stats);
  auto mempool_balance = GetChainBalance(address_stats.mempool_stats);

  uint64_t address_total =
      base::ClampSub(chain_balance.funded + mempool_balance.funded,
                     chain_balance.spent + mempool_balance.spent);
  if (address_total) {
    balance->balances[address_stats.address] = address_total;
  }

  balance->total_balance += address_total;
  balance->available_balance +=
      base::ClampSub(chain_balance.funded,
                     chain_balance.spent + mempool_balance.spent)
          .RawValue();
  balance->pending_balance += mempool_balance.funded;
  balance->pending_balance -= mempool_balance.spent;
}
}  // namespace brave_wallet
