/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_TEST_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_TEST_UTILS_H_

#include <string>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gmock/include/gmock/gmock-matchers.h"

namespace brave_wallet {

namespace test {

namespace internal {

template <typename T>
void AddToVector(std::vector<T>& v) {}

template <typename T, typename... Args>
void AddToVector(std::vector<T>& v, T&& first, Args&&... args) {
  v.push_back(std::forward<T>(first));
  AddToVector(v, std::forward<Args>(args)...);
}

}  // namespace internal

// Replacement of inplace vector creation with initializer list for non-copyable
// types.
template <typename... Args>
auto MakeVectorFromArgs(Args&&... args) {
  std::vector<std::common_type_t<Args...>> result;
  internal::AddToVector(result, std::forward<Args>(args)...);
  return result;
}

template <size_t SZ>
std::array<uint8_t, SZ> HexToArray(std::string_view input) {
  std::array<uint8_t, SZ> result;
  CHECK(base::HexStringToSpan(input, result));
  return result;
}

}  // namespace test

inline constexpr mojom::CoinType kAllCoins[] = {
    mojom::CoinType::ETH, mojom::CoinType::FIL, mojom::CoinType::SOL,
    mojom::CoinType::BTC, mojom::CoinType::ZEC, mojom::CoinType::ADA,
};

inline constexpr mojom::KeyringId kAllKeyrings[] = {
    mojom::KeyringId::kDefault,
    mojom::KeyringId::kBitcoin84,
    mojom::KeyringId::kBitcoin84Testnet,
    mojom::KeyringId::kFilecoin,
    mojom::KeyringId::kFilecoinTestnet,
    mojom::KeyringId::kSolana,
    mojom::KeyringId::kZCashMainnet,
    mojom::KeyringId::kZCashTestnet,
    mojom::KeyringId::kBitcoinImport,
    mojom::KeyringId::kBitcoinImportTestnet,
    mojom::KeyringId::kZCashMainnet,
    mojom::KeyringId::kZCashTestnet,
    mojom::KeyringId::kCardanoMainnet,
    mojom::KeyringId::kCardanoTestnet,
};

// Change calling test's hardcoded value only after it has adequate testing for
// newly added coin.
template <size_t N>
constexpr bool AllCoinsTested() {
  return N == std::size(kAllCoins);
}

// Change calling test's hardcoded value only after it has adequate testing for
// newly added keyring.
template <size_t N>
constexpr bool AllKeyringsTested() {
  return N == std::size(kAllKeyrings);
}

inline constexpr char kHttpURL[] = "http://bad.com/";
inline constexpr char kHttpLocalhostURL[] = "http://localhost:8080/";

mojom::NetworkInfo GetTestNetworkInfo1(
    const std::string& chain_id = "chain_id",
    mojom::CoinType coin = mojom::CoinType::ETH);
mojom::NetworkInfo GetTestNetworkInfo2(
    const std::string& chain_id = "chain_id2",
    mojom::CoinType coin = mojom::CoinType::ETH);
mojom::NetworkInfo GetTestNetworkInfoWithHttpURL(
    const std::string& chain_id = "http_url",
    mojom::CoinType coin = mojom::CoinType::ETH);
mojom::ChainIdPtr EthMainnetChainId();
mojom::ChainIdPtr SolMainnetChainId();

// Matcher to check equality of two mojo structs. Matcher needs copyable value
// which is not possible for some mojo types, so wrapping it with RefCounted.
template <typename T>
auto EqualsMojo(const T& value) {
  return testing::Truly(
      [value = base::MakeRefCounted<base::RefCountedData<T>>(value.Clone())](
          const T& candidate) { return mojo::Equals(candidate, value->data); });
}

namespace mojom {

// These are pretty printers for gmock expect/assert failures.
void PrintTo(const BitcoinAddressPtr& address, ::std::ostream* os);
void PrintTo(const BlockchainTokenPtr& token, ::std::ostream* os);
void PrintTo(const BitcoinBalancePtr& balance, ::std::ostream* os);
void PrintTo(const BitcoinKeyId& key_id, ::std::ostream* os);
void PrintTo(const BitcoinAccountInfoPtr& account_info, ::std::ostream* os);
void PrintTo(const BtcHardwareTransactionSignInputDataPtr& input_data,
             ::std::ostream* os);
void PrintTo(const CardanoAddressPtr& address, ::std::ostream* os);
void PrintTo(const CardanoBalancePtr& balance, ::std::ostream* os);

}  // namespace mojom

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_TEST_UTILS_H_
