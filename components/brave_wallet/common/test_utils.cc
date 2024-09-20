/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/test_utils.h"

#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"

namespace brave_wallet {

bool AllCoinsTested() {
  // Change hardcoded value here only when all failed callers have adequate
  // testing for newly added coin.
  return 5 == std::size(kAllCoins);
}

bool AllKeyringsTested() {
  // Change hardcoded value here only when all failed callers have adequate
  // testing for newly added keyring.
  return 12 == std::size(kAllKeyrings);
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
          GetSupportedKeyringsForNetwork(coin, chain_id)};
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
          GetSupportedKeyringsForNetwork(coin, chain_id)};
}

mojom::NetworkInfo GetTestNetworkInfoWithHttpURL(const std::string& chain_id,
                                                 mojom::CoinType coin) {
  return {chain_id,
          "invalid_url",
          {kHttpURL, kHttpLocalhostURL, "https://good.com"},
          {kHttpURL, kHttpLocalhostURL, "https://good.com"},
          0,
          {GURL("https://good.com"), GURL(kHttpURL), GURL(kHttpLocalhostURL)},
          "symbol2",
          "symbol_name2",
          22,
          coin,
          GetSupportedKeyringsForNetwork(coin, chain_id)};
}

namespace mojom {

void PrintTo(const BitcoinAddressPtr& address, ::std::ostream* os) {
  *os << base::StringPrintf("[%s %d/%d]", address->address_string.c_str(),
                            address->key_id->change, address->key_id->index);
}

void PrintTo(const BlockchainTokenPtr& token, ::std::ostream* os) {
  *os << BlockchainTokenToValue(token).DebugString();
}

void PrintTo(const BitcoinBalancePtr& balance, ::std::ostream* os) {
  *os << balance->total_balance << "/" << balance->available_balance << "/"
      << balance->pending_balance << std::endl;
  for (auto& address : balance->balances) {
    *os << address.first << "=" << address.second << std::endl;
  }
}

void PrintTo(const BitcoinKeyId& key_id, ::std::ostream* os) {
  *os << key_id.change << "/" << key_id.index;
}

void PrintTo(const BitcoinAccountInfoPtr& account_info, ::std::ostream* os) {
  PrintTo(account_info->next_receive_address, os);
  *os << "/";
  PrintTo(account_info->next_change_address, os);
}

void PrintTo(const BtcHardwareTransactionSignInputDataPtr& input_data,
             ::std::ostream* os) {
  *os << input_data->output_index << "/"
      << base::HexEncode(input_data->tx_bytes) << "/"
      << input_data->associated_path;
}

}  // namespace mojom

}  // namespace brave_wallet
