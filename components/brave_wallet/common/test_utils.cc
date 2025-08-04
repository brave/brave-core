/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/test_utils.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "third_party/abseil-cpp/absl/strings/str_format.h"

namespace brave_wallet {

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

mojom::ChainIdPtr EthMainnetChainId() {
  return mojom::ChainId::New(mojom::CoinType::ETH, mojom::kMainnetChainId);
}

mojom::ChainIdPtr SolMainnetChainId() {
  return mojom::ChainId::New(mojom::CoinType::SOL, mojom::kSolanaMainnet);
}

namespace mojom {

void PrintTo(const BitcoinAddressPtr& address, ::std::ostream* os) {
  *os << absl::StrFormat("[%s %d/%d]", address->address_string,
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

void PrintTo(const CardanoAddressPtr& address, ::std::ostream* os) {
  *os << absl::StrFormat("[%s %d/%d]", address->address_string,
                         address->payment_key_id->role,
                         address->payment_key_id->index);
}

void PrintTo(const CardanoBalancePtr& balance, ::std::ostream* os) {
  *os << absl::StrFormat("[%d]", balance->total_balance);
}

}  // namespace mojom

}  // namespace brave_wallet
