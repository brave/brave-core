/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/network_manager.h"

#include <optional>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/check_op.h"
#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/containers/extend.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace brave_wallet {

namespace {

std::optional<bool> GetEip1559ForKnownChain(std::string_view chain_id_lwr) {
  static base::NoDestructor<base::flat_map<std::string_view, bool>> values([] {
    base::flat_map<std::string_view, bool> values({
        {mojom::kMainnetChainId, true},  //
        {mojom::kPolygonMainnetChainId, true},
        {mojom::kAvalancheMainnetChainId, true},
        {mojom::kOptimismMainnetChainId, true},  //
        {mojom::kSepoliaChainId, true},          //
        {mojom::kFilecoinEthereumMainnetChainId, true},
        {mojom::kFilecoinEthereumTestnetChainId, true},
        {mojom::kBnbSmartChainMainnetChainId, false},
        {mojom::kBaseMainnetChainId, true},
        {mojom::kNeonEVMMainnetChainId, false},
        {mojom::kLocalhostChainId, false},
    });
    return values;
  }());

  auto it = values->find(chain_id_lwr);
  if (it == values->end()) {
    return std::nullopt;
  }

  return it->second;
}

constexpr char kGanacheLocalhostURL[] = "http://localhost:7545/";
constexpr char kSolanaLocalhostURL[] = "http://localhost:8899/";
constexpr char kFilecoinLocalhostURL[] = "http://localhost:1234/rpc/v0";

const std::string GetChainSubdomain(std::string_view chain_id) {
  static base::NoDestructor<base::flat_map<std::string, std::string>>
      subdomains({
          // EVM chains.
          {mojom::kMainnetChainId, "ethereum-mainnet"},
          {mojom::kSepoliaChainId, "ethereum-sepolia"},
          {mojom::kPolygonMainnetChainId, "polygon-mainnet"},
          {mojom::kOptimismMainnetChainId, "optimism-mainnet"},
          {mojom::kBaseMainnetChainId, "base-mainnet"},
          {mojom::kAvalancheMainnetChainId, "avalanche-mainnet"},
          {mojom::kBnbSmartChainMainnetChainId, "bsc-mainnet"},

          // SVM chains.
          {mojom::kSolanaMainnet, "solana-mainnet"},

          // Bitcoin chains.
          {mojom::kBitcoinMainnet, "bitcoin-mainnet"},

          // Cardano chains.
          {mojom::kCardanoMainnet, "cardano-mainnet"},
          {mojom::kCardanoTestnet, "cardano-preprod"},
      });

  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (subdomains->contains(chain_id_lower)) {
    return subdomains->at(chain_id_lower);
  }

  return "";
}

std::optional<GURL> GetURLForKnownChainId(std::string_view chain_id) {
  auto subdomain = brave_wallet::GetChainSubdomain(chain_id);
  if (subdomain.empty()) {
    return std::nullopt;
  }
  return GURL(
      base::StringPrintf("https://%s.wallet.brave.com", subdomain.c_str()));
}

const mojom::NetworkInfo* GetEthMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Ethereum Mainnet",
       {"https://etherscan.io"},
       {},
       0,
       {GetURLForKnownChainId(chain_id).value()},
       "ETH",
       "Ethereum",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetPolygonMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kPolygonMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Polygon Mainnet",
       {"https://polygonscan.com"},
       {},
       0,
       {GetURLForKnownChainId(chain_id).value()},
       "MATIC",
       "MATIC",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetBscMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kBnbSmartChainMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "BNB Smart Chain",
       {"https://bscscan.com"},
       {},
       0,
       {GetURLForKnownChainId(chain_id).value()},
       "BNB",
       "BNB",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetAvalancheMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kAvalancheMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Avalanche C-Chain",
       {"https://snowtrace.io"},
       {},
       0,
       {GetURLForKnownChainId(chain_id).value()},
       "AVAX",
       "Avalanche",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetOptimismMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kOptimismMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Optimism",
       {"https://optimistic.etherscan.io"},
       {},
       0,
       {GetURLForKnownChainId(chain_id).value()},
       "ETH",
       "Ether",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetBaseMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kBaseMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Base",
       {"https://basescan.org"},
       {},
       0,
       {GetURLForKnownChainId(chain_id).value()},
       "ETH",
       "Ether",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetNeonEVMMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kNeonEVMMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Neon EVM",
       {"https://neonscan.org"},
       {},
       0,
       {GURL("https://neon-proxy-mainnet.solana.p2p.org")},
       "NEON",
       "Neon",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetSepoliaTestNetwork() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kSepoliaChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Sepolia Test Network",
       {"https://sepolia.etherscan.io"},
       {},
       0,
       {GetURLForKnownChainId(chain_id).value()},
       "ETH",
       "Ethereum",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetEthLocalhost() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kLocalhostChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Localhost",
       {kGanacheLocalhostURL},
       {},
       0,
       {GURL(kGanacheLocalhostURL)},
       "ETH",
       "Ethereum",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetFilecoinEthereumMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kFilecoinEthereumMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Filecoin EVM Mainnet",
       {"https://filfox.info/en/message"},
       {},
       0,
       {GURL("https://api.node.glif.io/rpc/v1")},
       "FIL",
       "Filecoin",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetFilecoinEthereumTestnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kFilecoinEthereumTestnetChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Filecoin EVM Testnet",
       {"https://calibration.filfox.info/en/message"},
       {},
       0,
       {GURL("https://api.calibration.node.glif.io/rpc/v1")},
       "FIL",
       "Filecoin",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

// Precompiled networks available in native wallet.
const std::vector<const mojom::NetworkInfo*>& GetKnownEthNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetEthMainnet(),
      GetBaseMainnet(),
      GetPolygonMainnet(),
      GetBscMainnet(),
      GetOptimismMainnet(),
      GetAvalancheMainnet(),
      GetFilecoinEthereumMainnet(),
      GetNeonEVMMainnet(),
      GetSepoliaTestNetwork(),
      GetFilecoinEthereumTestnet(),
      GetEthLocalhost()
      // clang-format on
  });
  return *networks.get();
}

const mojom::NetworkInfo* GetSolMainnet() {
  const auto coin = mojom::CoinType::SOL;
  const auto* chain_id = mojom::kSolanaMainnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Solana Mainnet Beta",
       {"https://explorer.solana.com/"},
       {},
       0,
       {GetURLForKnownChainId(chain_id).value()},
       "SOL",
       "Solana",
       9,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetSolTestnet() {
  const auto coin = mojom::CoinType::SOL;
  const auto* chain_id = mojom::kSolanaTestnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Solana Testnet",
       {"https://explorer.solana.com/?cluster=testnet"},
       {},
       0,
       {GURL("https://api.testnet.solana.com")},
       "SOL",
       "Solana",
       9,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetSolDevnet() {
  const auto coin = mojom::CoinType::SOL;
  const auto* chain_id = mojom::kSolanaDevnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Solana Devnet",
       {"https://explorer.solana.com/?cluster=devnet"},
       {},
       0,
       {GURL("https://api.devnet.solana.com")},
       "SOL",
       "Solana",
       9,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetSolLocalhost() {
  const auto coin = mojom::CoinType::SOL;
  const auto* chain_id = mojom::kLocalhostChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Solana Localhost",
       {"https://explorer.solana.com/"
        "?cluster=custom&customUrl=http%3A%2F%2Flocalhost%3A8899"},
       {},
       0,
       {GURL(kSolanaLocalhostURL)},
       "SOL",
       "Solana",
       9,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const std::vector<const mojom::NetworkInfo*>& GetKnownSolNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetSolMainnet(),
      GetSolTestnet(),
      GetSolDevnet(),
      GetSolLocalhost(),
      // clang-format on
  });
  return *networks.get();
}

const mojom::NetworkInfo* GetFilMainnet() {
  const auto coin = mojom::CoinType::FIL;
  const auto* chain_id = mojom::kFilecoinMainnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Filecoin Mainnet",
       {"https://filscan.io/tipset/message-detail"},
       {},
       0,
       {GURL("https://api.node.glif.io/rpc/v0")},
       "FIL",
       "Filecoin",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetFilTestnet() {
  const auto coin = mojom::CoinType::FIL;
  const auto* chain_id = mojom::kFilecoinTestnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Filecoin Testnet",
       {"https://calibration.filscan.io/tipset/message-detail"},
       {},
       0,
       {GURL("https://api.calibration.node.glif.io/rpc/v0")},
       "FIL",
       "Filecoin",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetFilLocalhost() {
  const auto coin = mojom::CoinType::FIL;
  const auto* chain_id = mojom::kLocalhostChainId;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Filecoin Localhost",
       {kFilecoinLocalhostURL},
       {},
       0,
       {GURL(kFilecoinLocalhostURL)},
       "FIL",
       "Filecoin",
       18,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const std::vector<const mojom::NetworkInfo*>& GetKnownFilNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetFilMainnet(),
      GetFilTestnet(),
      GetFilLocalhost(),
      // clang-format on
  });
  return *networks.get();
}

GURL ZCashMainnetRpcUrl() {
  auto switch_url =
      GURL(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kZCashMainnetRpcUrl));
  if (switch_url.is_valid()) {
    return switch_url;
  }
  return GURL("https://zcash.wallet.brave.com/");
}

GURL ZCashTestnetRpcUrl() {
  auto switch_url =
      GURL(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kZCashTestnetRpcUrl));
  if (switch_url.is_valid()) {
    return switch_url;
  }
  return GURL("https://testnet.lightwalletd.com:9067/");
}

GURL CardanoMainnetRpcUrl() {
  auto switch_url =
      GURL(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kCardanoMainnetRpcUrl));
  if (switch_url.is_valid()) {
    return switch_url;
  }
  return GetURLForKnownChainId(mojom::kCardanoMainnet).value();
}

GURL CardanoTestnetRpcUrl() {
  auto switch_url =
      GURL(base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kCardanoTestnetRpcUrl));
  if (switch_url.is_valid()) {
    return switch_url;
  }
  return GetURLForKnownChainId(mojom::kCardanoTestnet).value();
}

const mojom::NetworkInfo* GetBitcoinMainnet() {
  const auto coin = mojom::CoinType::BTC;
  const auto* chain_id = mojom::kBitcoinMainnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Bitcoin Mainnet",
       {"https://www.blockchain.com/explorer"},
       {},
       0,
       {GetURLForKnownChainId(chain_id).value()},
       "BTC",
       "Bitcoin",
       8,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetBitcoinTestnet() {
  const auto coin = mojom::CoinType::BTC;
  const auto* chain_id = mojom::kBitcoinTestnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Bitcoin Testnet",
       {"https://blockstream.info/testnet"},
       {},
       0,
       {GURL(kBitcoinTestnetRpcEndpoint)},
       "BTC",
       "Bitcoin",
       8,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetZCashMainnet() {
  const auto coin = mojom::CoinType::ZEC;
  const auto* chain_id = mojom::kZCashMainnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Zcash Mainnet",
       {"https://3xpl.com/zcash/transaction"},
       {},
       0,
       {ZCashMainnetRpcUrl()},
       "ZEC",
       "Zcash",
       8,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetZCashTestnet() {
  const auto coin = mojom::CoinType::ZEC;
  const auto* chain_id = mojom::kZCashTestnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Zcash Testnet",
       {"https://blockexplorer.one/zcash/testnet/tx"},
       {},
       0,
       {ZCashTestnetRpcUrl()},
       "ZEC",
       "Zcash",
       8,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetCardanoMainnet() {
  const auto coin = mojom::CoinType::ADA;
  const auto* chain_id = mojom::kCardanoMainnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Cardano Mainnet",
       {"https://cexplorer.io"},
       {},
       0,
       {CardanoMainnetRpcUrl()},
       "ADA",
       "Cardano",
       6,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const mojom::NetworkInfo* GetCardanoTestnet() {
  const auto coin = mojom::CoinType::ADA;
  const auto* chain_id = mojom::kCardanoTestnet;

  static base::NoDestructor<mojom::NetworkInfo> network_info(
      {chain_id,
       "Cardano Preprod Testnet",
       {"https://preprod.cexplorer.io"},
       {},
       0,
       {CardanoTestnetRpcUrl()},
       "ADA",
       "Cardano",
       6,
       coin,
       GetSupportedKeyringsForNetwork(coin, chain_id)});
  return network_info.get();
}

const std::vector<const mojom::NetworkInfo*>& GetKnownBitcoinNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetBitcoinMainnet(),
      GetBitcoinTestnet(),
      // clang-format on
  });
  return *networks.get();
}

const std::vector<const mojom::NetworkInfo*>& GetKnownZCashNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetZCashMainnet(),
      GetZCashTestnet(),
      // clang-format on
  });
  return *networks.get();
}

const std::vector<const mojom::NetworkInfo*>& GetKnownCardanoNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetCardanoMainnet(),
      GetCardanoTestnet(),
      // clang-format on
  });
  return *networks.get();
}

std::string GetPrefKeyForCoinType(mojom::CoinType coin) {
  switch (coin) {
    case mojom::CoinType::BTC:
      return kBitcoinPrefKey;
    case mojom::CoinType::ZEC:
      return kZCashPrefKey;
    case mojom::CoinType::ETH:
      return kEthereumPrefKey;
    case mojom::CoinType::FIL:
      return kFilecoinPrefKey;
    case mojom::CoinType::SOL:
      return kSolanaPrefKey;
    case mojom::CoinType::ADA:
      return kCardanoPrefKey;
  }
  NOTREACHED() << coin;
}

const base::Value::List* GetCustomNetworksList(PrefService* prefs,
                                               mojom::CoinType coin) {
  const auto& custom_networks = prefs->GetDict(kBraveWalletCustomNetworks);
  return custom_networks.FindList(GetPrefKeyForCoinType(coin));
}

std::vector<mojom::NetworkInfoPtr> MergeKnownAndCustomChains(
    std::vector<mojom::NetworkInfoPtr> known_chains,
    std::vector<mojom::NetworkInfoPtr> custom_chains) {
  std::vector<mojom::NetworkInfoPtr> result;
  // Put all known chains to result, replace with custom one if chain_id
  // matches.
  for (auto& known_chain : known_chains) {
    bool add_known_chain = true;
    for (auto& custom_chain : custom_chains) {
      if (custom_chain &&
          base::CompareCaseInsensitiveASCII(custom_chain->chain_id,
                                            known_chain->chain_id) == 0) {
        result.push_back(std::move(custom_chain));
        add_known_chain = false;
        break;
      }
    }
    if (add_known_chain) {
      result.push_back(std::move(known_chain));
    }
  }

  // Put all remaining custom chains to result.
  for (auto& custom_chain : custom_chains) {
    if (custom_chain) {
      result.push_back(std::move(custom_chain));
    }
  }

  return result;
}

std::string GetCurrentChainIdFromPrefs(PrefService* prefs,
                                       mojom::CoinType coin) {
  const auto& selected_networks = prefs->GetDict(kBraveWalletSelectedNetworks);
  const std::string* chain_id =
      selected_networks.FindString(GetPrefKeyForCoinType(coin));
  if (!chain_id) {
    return std::string();
  }

  return base::ToLowerASCII(*chain_id);
}

std::string GetCurrentChainIdFromPrefs(
    PrefService* prefs,
    mojom::CoinType coin,
    const std::optional<::url::Origin>& origin) {
  if (!origin) {
    return GetCurrentChainIdFromPrefs(prefs, coin);
  }
  const auto& selected_networks =
      prefs->GetDict(kBraveWalletSelectedNetworksPerOrigin);
  const auto* coin_dict =
      selected_networks.FindDict(GetPrefKeyForCoinType(coin));
  if (!coin_dict) {
    return GetCurrentChainIdFromPrefs(prefs, coin);
  }
  const auto* chain_id_str = coin_dict->FindString(origin->Serialize());
  if (!chain_id_str) {
    return GetCurrentChainIdFromPrefs(prefs, coin);
  }

  return base::ToLowerASCII(*chain_id_str);
}

}  // namespace

NetworkManager::NetworkManager(PrefService* prefs) : prefs_(prefs) {}
NetworkManager::~NetworkManager() = default;

// static
mojom::NetworkInfoPtr NetworkManager::GetKnownChain(std::string_view chain_id,
                                                    mojom::CoinType coin) {
  if (coin == mojom::CoinType::ETH) {
    for (const auto* network : GetKnownEthNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return network->Clone();
      }
    }
    return nullptr;
  }

  if (coin == mojom::CoinType::FIL) {
    for (const auto* network : GetKnownFilNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return network->Clone();
      }
    }
    return nullptr;
  }

  if (coin == mojom::CoinType::SOL) {
    for (const auto* network : GetKnownSolNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return network->Clone();
      }
    }
    return nullptr;
  }

  if (coin == mojom::CoinType::BTC) {
    for (const auto* network : GetKnownBitcoinNetworks()) {
      if (base::EqualsCaseInsensitiveASCII(network->chain_id, chain_id)) {
        return network->Clone();
      }
    }
    return nullptr;
  }

  if (coin == mojom::CoinType::ZEC) {
    for (const auto* network : GetKnownZCashNetworks()) {
      if (base::EqualsCaseInsensitiveASCII(network->chain_id, chain_id)) {
        return network->Clone();
      }
    }
    return nullptr;
  }

  if (coin == mojom::CoinType::ADA) {
    for (const auto* network : GetKnownCardanoNetworks()) {
      if (base::EqualsCaseInsensitiveASCII(network->chain_id, chain_id)) {
        return network->Clone();
      }
    }
    return nullptr;
  }

  NOTREACHED() << coin;
}

mojom::NetworkInfoPtr NetworkManager::GetCustomChain(std::string_view chain_id,
                                                     mojom::CoinType coin) {
  const base::Value::List* custom_list = GetCustomNetworksList(prefs_, coin);
  if (!custom_list) {
    return nullptr;
  }
  for (const auto& it : *custom_list) {
    if (auto opt_chain_id =
            brave_wallet::ExtractChainIdFromValue(it.GetIfDict())) {
      if (base::CompareCaseInsensitiveASCII(chain_id, *opt_chain_id) == 0) {
        return brave_wallet::ValueToNetworkInfo(it);
      }
    }
  }
  return nullptr;
}

mojom::NetworkInfoPtr NetworkManager::GetChain(std::string_view chain_id,
                                               mojom::CoinType coin) {
  if (chain_id.empty()) {
    return nullptr;
  }
  if (auto custom_chain = GetCustomChain(chain_id, coin)) {
    DCHECK(!custom_chain->supported_keyrings.empty());
    return custom_chain;
  }
  if (auto known_chain = GetKnownChain(chain_id, coin)) {
    DCHECK(!known_chain->supported_keyrings.empty());
    return known_chain;
  }

  return nullptr;
}

std::vector<mojom::NetworkInfoPtr> NetworkManager::GetAllCustomChains(
    mojom::CoinType coin) {
  std::vector<mojom::NetworkInfoPtr> result;
  auto* custom_list = GetCustomNetworksList(prefs_, coin);
  if (!custom_list) {
    return result;
  }

  for (const auto& it : *custom_list) {
    mojom::NetworkInfoPtr chain = ValueToNetworkInfo(it);
    if (chain) {
      DCHECK_EQ(chain->coin, coin);
      result.push_back(std::move(chain));
    }
  }

  return result;
}

bool NetworkManager::KnownChainExists(std::string_view chain_id,
                                      mojom::CoinType coin) {
  if (coin == mojom::CoinType::ETH) {
    for (const auto* network : GetKnownEthNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return true;
      }
    }
  } else if (coin == mojom::CoinType::SOL) {
    for (const auto* network : GetKnownSolNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return true;
      }
    }
  } else if (coin == mojom::CoinType::FIL) {
    for (const auto* network : GetKnownFilNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return true;
      }
    }
  } else if (coin == mojom::CoinType::BTC) {
    for (const auto* network : GetKnownBitcoinNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return true;
      }
    }
  } else if (coin == mojom::CoinType::ZEC) {
    for (const auto* network : GetKnownZCashNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return true;
      }
    }
  } else if (coin == mojom::CoinType::ADA) {
    for (const auto* network : GetKnownCardanoNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return true;
      }
    }
  } else {
    NOTREACHED() << coin;
  }
  return false;
}

bool NetworkManager::CustomChainExists(std::string_view custom_chain_id,
                                       mojom::CoinType coin) {
  const base::Value::List* custom_list = GetCustomNetworksList(prefs_, coin);
  if (!custom_list) {
    return false;
  }
  for (const auto& it : *custom_list) {
    if (auto chain_id = ExtractChainIdFromValue(it.GetIfDict())) {
      if (base::CompareCaseInsensitiveASCII(*chain_id, custom_chain_id) == 0) {
        return true;
      }
    }
  }
  return false;
}

// Returns the subset of custom_chain_ids that are custom chains
std::vector<std::string> NetworkManager::CustomChainsExist(
    const std::vector<std::string>& custom_chain_ids,
    mojom::CoinType coin) {
  const base::Value::List* custom_list = GetCustomNetworksList(prefs_, coin);
  std::vector<std::string> existing_chain_ids;

  if (!custom_list) {
    return existing_chain_ids;
  }

  for (const auto& it : *custom_list) {
    if (auto chain_id = ExtractChainIdFromValue(it.GetIfDict())) {
      for (const auto& custom_chain_id : custom_chain_ids) {
        if (base::CompareCaseInsensitiveASCII(*chain_id, custom_chain_id) ==
            0) {
          existing_chain_ids.push_back(custom_chain_id);
          break;
        }
      }
    }
  }
  return existing_chain_ids;
}

// static
GURL NetworkManager::GetUnstoppableDomainsRpcUrl(std::string_view chain_id) {
  if (chain_id == mojom::kMainnetChainId) {
    return GetEthMainnet()->rpc_endpoints.front();
  } else if (chain_id == mojom::kPolygonMainnetChainId) {
    return GetPolygonMainnet()->rpc_endpoints.front();
  } else if (chain_id == mojom::kBaseMainnetChainId) {
    return GetBaseMainnet()->rpc_endpoints.front();
  }

  NOTREACHED();
}

// static
GURL NetworkManager::GetEnsRpcUrl() {
  return GetEthMainnet()->rpc_endpoints.front();
}

// static
GURL NetworkManager::GetSnsRpcUrl() {
  return GetSolMainnet()->rpc_endpoints.front();
}

std::vector<mojom::NetworkInfoPtr> NetworkManager::GetAllKnownChains(
    mojom::CoinType coin) {
  std::vector<mojom::NetworkInfoPtr> result;

  if (coin == mojom::CoinType::ETH) {
    for (const auto* network : GetKnownEthNetworks()) {
      result.push_back(network->Clone());
    }
    return result;
  }

  if (coin == mojom::CoinType::SOL) {
    for (const auto* network : GetKnownSolNetworks()) {
      result.push_back(network->Clone());
    }
    return result;
  }

  if (coin == mojom::CoinType::FIL) {
    for (const auto* network : GetKnownFilNetworks()) {
      result.push_back(network->Clone());
    }
    return result;
  }

  if (coin == mojom::CoinType::BTC) {
    for (const auto* network : GetKnownBitcoinNetworks()) {
      result.push_back(network->Clone());
    }
    return result;
  }

  if (coin == mojom::CoinType::ZEC) {
    for (const auto* network : GetKnownZCashNetworks()) {
      result.push_back(network->Clone());
    }
    return result;
  }

  if (coin == mojom::CoinType::ADA) {
    for (const auto* network : GetKnownCardanoNetworks()) {
      result.push_back(network->Clone());
    }
    return result;
  }

  NOTREACHED() << coin;
}

GURL NetworkManager::GetNetworkURL(std::string_view chain_id,
                                   mojom::CoinType coin) {
  if (network_url_for_testing_.contains(std::string(chain_id))) {
    return network_url_for_testing_.at(std::string(chain_id));
  }

  if (auto custom_chain = GetCustomChain(chain_id, coin)) {
    return GetActiveEndpointUrl(*custom_chain);
  }

  if (auto known_chain = GetKnownChain(chain_id, coin)) {
    return GetActiveEndpointUrl(*known_chain);
  }
  return GURL();
}

GURL NetworkManager::GetNetworkURL(mojom::CoinType coin,
                                   const std::optional<url::Origin>& origin) {
  return GetNetworkURL(GetCurrentChainId(coin, origin), coin);
}

std::vector<mojom::NetworkInfoPtr> NetworkManager::GetAllChains() {
  std::vector<mojom::NetworkInfoPtr> result;
  for (auto coin : GetEnabledCoins()) {
    base::Extend(result, MergeKnownAndCustomChains(GetAllKnownChains(coin),
                                                   GetAllCustomChains(coin)));
  }
  return result;
}

std::optional<bool> NetworkManager::IsEip1559Chain(

    std::string_view chain_id) {
  auto chain_id_lwr = base::ToLowerASCII(chain_id);
  if (auto is_eip_1559 = prefs_->GetDict(kBraveWalletEip1559CustomChains)
                             .FindBool(chain_id_lwr)) {
    return is_eip_1559.value();
  }
  return GetEip1559ForKnownChain(chain_id_lwr);
}

void NetworkManager::SetEip1559ForCustomChain(std::string_view chain_id,
                                              std::optional<bool> is_eip1559) {
  auto chain_id_lwr = base::ToLowerASCII(chain_id);
  ScopedDictPrefUpdate update(prefs_, kBraveWalletEip1559CustomChains);
  if (is_eip1559.has_value()) {
    update->Set(chain_id_lwr, is_eip1559.value());
  } else {
    update->Remove(chain_id_lwr);
  }
}

void NetworkManager::AddCustomNetwork(const mojom::NetworkInfo& chain) {
  ScopedDictPrefUpdate update(prefs_, kBraveWalletCustomNetworks);
  update->EnsureList(GetPrefKeyForCoinType(chain.coin))
      ->Append(NetworkInfoToValue(chain));
}

void NetworkManager::RemoveCustomNetwork(std::string_view chain_id,
                                         mojom::CoinType coin) {
  ScopedDictPrefUpdate update(prefs_, kBraveWalletCustomNetworks);
  base::Value::List* list = update->FindList(GetPrefKeyForCoinType(coin));
  if (!list) {
    return;
  }
  bool removed = list->EraseIf([&chain_id](const base::Value& v) {
    DCHECK(v.is_dict());
    auto* chain_id_value = v.GetDict().FindString("chainId");
    if (!chain_id_value) {
      return false;
    }
    return base::EqualsCaseInsensitiveASCII(*chain_id_value, chain_id);
  });

  if (removed) {
    SetEip1559ForCustomChain(chain_id, std::nullopt);
  }
}

std::vector<std::string> NetworkManager::GetHiddenNetworks(
    mojom::CoinType coin) {
  std::vector<std::string> result;
  const auto& hidden_networks = prefs_->GetDict(kBraveWalletHiddenNetworks);

  auto* hidden_networks_list =
      hidden_networks.FindList(GetPrefKeyForCoinType(coin));
  if (!hidden_networks_list) {
    return result;
  }

  for (const auto& it : *hidden_networks_list) {
    if (auto* chain_id = it.GetIfString()) {
      result.push_back(base::ToLowerASCII(*chain_id));
    }
  }

  return result;
}

void NetworkManager::AddHiddenNetwork(mojom::CoinType coin,
                                      std::string_view chain_id) {
  ScopedDictPrefUpdate update(prefs_, kBraveWalletHiddenNetworks);
  base::Value::List* list = update->EnsureList(GetPrefKeyForCoinType(coin));
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (!base::Contains(*list, base::Value(chain_id_lower))) {
    list->Append(chain_id_lower);
  }
}

void NetworkManager::RemoveHiddenNetwork(mojom::CoinType coin,
                                         std::string_view chain_id) {
  ScopedDictPrefUpdate update(prefs_, kBraveWalletHiddenNetworks);
  base::Value::List* list = update->FindList(GetPrefKeyForCoinType(coin));
  if (!list) {
    return;
  }
  list->EraseIf([&](const base::Value& v) {
    auto* chain_id_string = v.GetIfString();
    if (!chain_id_string) {
      return false;
    }
    return base::CompareCaseInsensitiveASCII(*chain_id_string, chain_id) == 0;
  });
}

std::string NetworkManager::GetCurrentChainId(
    mojom::CoinType coin,
    const std::optional<url::Origin>& origin) {
  auto chain_id_from_prefs = GetCurrentChainIdFromPrefs(prefs_, coin, origin);
  for (auto& chain : GetAllChains()) {
    if (coin == chain->coin && base::EqualsCaseInsensitiveASCII(
                                   chain_id_from_prefs, chain->chain_id)) {
      return chain_id_from_prefs;
    }
  }
  switch (coin) {
    case mojom::CoinType::ETH:
      return mojom::kMainnetChainId;
    case mojom::CoinType::SOL:
      return mojom::kSolanaMainnet;
    case mojom::CoinType::FIL:
      return mojom::kFilecoinMainnet;
    case mojom::CoinType::BTC:
      return mojom::kBitcoinMainnet;
    case mojom::CoinType::ZEC:
      return mojom::kZCashMainnet;
    case mojom::CoinType::ADA:
      return mojom::kCardanoMainnet;
  }
  NOTREACHED() << coin;
}

bool NetworkManager::SetCurrentChainId(mojom::CoinType coin,
                                       const std::optional<url::Origin>& origin,
                                       std::string_view chain_id) {
  // We cannot switch to an unknown chain_id
  if (!KnownChainExists(chain_id, coin) && !CustomChainExists(chain_id, coin)) {
    return false;
  }
  if (!origin) {
    ScopedDictPrefUpdate update(prefs_, kBraveWalletSelectedNetworks);
    update->Set(GetPrefKeyForCoinType(coin), chain_id);
  } else {
    if (origin->opaque()) {
      return false;
    }
    // Only set per origin network for http/https scheme
    if (origin->scheme() == url::kHttpScheme ||
        origin->scheme() == url::kHttpsScheme) {
      ScopedDictPrefUpdate update(prefs_,
                                  kBraveWalletSelectedNetworksPerOrigin);
      update->EnsureDict(GetPrefKeyForCoinType(coin))
          ->Set(origin->Serialize(), chain_id);
    } else {
      ScopedDictPrefUpdate update(prefs_, kBraveWalletSelectedNetworks);
      update->Set(GetPrefKeyForCoinType(coin), chain_id);
    }
  }
  return true;
}

void NetworkManager::SetNetworkURLForTesting(const std::string& chain_id,
                                             GURL url) {
  CHECK_IS_TEST();
  if (url.is_valid()) {
    network_url_for_testing_[chain_id] = std::move(url);
  } else {
    network_url_for_testing_.erase(chain_id);
  }
}

}  // namespace brave_wallet
