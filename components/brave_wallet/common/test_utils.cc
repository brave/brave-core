/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/test_utils.h"

#include "brave/components/brave_wallet/common/common_utils.h"

namespace brave_wallet {

bool AllCoinsTested() {
  // Change harcoded value here only when all failed callers have adequate
  // testing for newly added coin.
  return 4 == std::size(kAllCoins);
}

mojom::NetworkInfo GetTestNetworkInfo1(const std::string& chain_id,
                                       mojom::CoinType coin) {
  return {chain_id,
          "chain_name",
          {"https://url1.com"},
          {"https://url1.com"},
          0,
          {GURL("https://url1.com")},
          "symbol",
          "symbol_name",
          11,
          coin,
          GetSupportedKeyringsForNetwork(coin, chain_id),
          false};
}

mojom::NetworkInfo GetTestNetworkInfo2(const std::string& chain_id,
                                       mojom::CoinType coin) {
  return {chain_id,
          "chain_name2",
          {"https://url2.com"},
          {"https://url2.com"},
          0,
          {GURL("https://url2.com")},
          "symbol2",
          "symbol_name2",
          22,
          coin,
          GetSupportedKeyringsForNetwork(coin, chain_id),
          true};
}

}  // namespace brave_wallet
