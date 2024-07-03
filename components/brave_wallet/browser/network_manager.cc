/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/network_manager.h"

#include <cstdlib>
#include <optional>
#include <utility>

#include "base/check.h"
#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/constants/brave_services_key.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "url/gurl.h"

namespace brave_wallet {

namespace {

std::optional<bool> GetEip1559ForKnownChain(const std::string& chain_id_lwr) {
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
        {mojom::kAuroraMainnetChainId, false},
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

const char kGanacheLocalhostURL[] = "http://localhost:7545/";
const char kSolanaLocalhostURL[] = "http://localhost:8899/";
const char kFilecoinLocalhostURL[] = "http://localhost:1234/rpc/v0";

const std::string GetChainSubdomain(const std::string& chain_id) {
  static base::NoDestructor<base::flat_map<std::string, std::string>>
      subdomains({// EVM chains
                  {mojom::kMainnetChainId, "ethereum-mainnet"},
                  {mojom::kSepoliaChainId, "ethereum-sepolia"},
                  {mojom::kPolygonMainnetChainId, "polygon-mainnet"},
                  {mojom::kOptimismMainnetChainId, "optimism-mainnet"},
                  {mojom::kAuroraMainnetChainId, "aurora-mainnet"},
                  {mojom::kAvalancheMainnetChainId, "avalanche-mainnet"},
                  {mojom::kBnbSmartChainMainnetChainId, "bsc-mainnet"},

                  // SVM chains
                  {mojom::kSolanaMainnet, "solana-mainnet"},

                  // Other chains
                  {mojom::kBitcoinMainnet, "bitcoin-mainnet"}});

  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (subdomains->contains(chain_id_lower)) {
    return subdomains->at(chain_id_lower);
  }

  return "";
}

std::optional<GURL> GetURLForKnownChainId(const std::string& chain_id) {
  auto subdomain = brave_wallet::GetChainSubdomain(chain_id);
  if (subdomain.empty()) {
    return std::nullopt;
  }
  return GURL(
      base::StringPrintf("https://%s.wallet.brave.com", subdomain.c_str()));
}

mojom::NetworkPropsPtr DefaultProps() {
  // To be filled by NetworkManager.
  return mojom::NetworkProps::New();
}

std::vector<GURL> RpcEndpointsForKnownChain(const std::string& chain_id) {
  auto url = GetURLForKnownChainId(chain_id);
  CHECK(url);
  return {*url};
}

std::vector<GURL> RpcEndpointsForURL(const std::string& url_spec) {
  GURL url(url_spec);
  CHECK(url.is_valid());
  return {std::move(url)};
}

std::vector<std::string> BlockExplorers(const std::string& url) {
  return {url};
}

std::vector<std::string> EmptyIconUrls() {
  return {};
}

const mojom::NetworkInfo* GetEthMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Ethereum Mainnet",
                BlockExplorers("https://etherscan.io"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForKnownChain(chain_id),
                "ETH",
                "Ethereum",
                18,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetPolygonMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kPolygonMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Polygon Mainnet",
                BlockExplorers("https://polygonscan.com"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForKnownChain(chain_id),
                "MATIC",
                "MATIC",
                18,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetBscMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kBnbSmartChainMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "BNB Smart Chain",
                BlockExplorers("https://bscscan.com"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForKnownChain(chain_id),
                "BNB",
                "BNB",
                18,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetAvalancheMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kAvalancheMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Avalanche C-Chain",
                BlockExplorers("https://snowtrace.io"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForKnownChain(chain_id),
                "AVAX",
                "Avalanche",
                18,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetOptimismMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kOptimismMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Optimism",
                BlockExplorers("https://optimistic.etherscan.io"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForKnownChain(chain_id),
                "ETH",
                "Ether",
                18,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetAuroraMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kAuroraMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Aurora Mainnet",
                BlockExplorers("https://aurorascan.dev"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForKnownChain(chain_id),
                "ETH",
                "Ether",
                18,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetNeonEVMMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kNeonEVMMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Neon EVM",
                BlockExplorers("https://neonscan.org"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForURL("https://neon-proxy-mainnet.solana.p2p.org"),
                "NEON",
                "Neon",
                18,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetSepoliaTestNetwork() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kSepoliaChainId;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Sepolia Test Network",
                BlockExplorers("https://sepolia.etherscan.io"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForKnownChain(chain_id),
                "ETH",
                "Ethereum",
                18,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetEthLocalhost() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kLocalhostChainId;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Localhost",
                BlockExplorers(kGanacheLocalhostURL),
                EmptyIconUrls(),
                0,
                RpcEndpointsForURL(kGanacheLocalhostURL),
                "ETH",
                "Ethereum",
                18,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetFilecoinEthereumMainnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kFilecoinEthereumMainnetChainId;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Filecoin EVM Mainnet",
                BlockExplorers("https://filfox.info/en/message"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForURL("https://api.node.glif.io/rpc/v1"),
                "FIL",
                "Filecoin",
                18,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetFilecoinEthereumTestnet() {
  const auto coin = mojom::CoinType::ETH;
  const auto* chain_id = mojom::kFilecoinEthereumTestnetChainId;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {
            std::in_place,
            chain_id,
            "Filecoin EVM Testnet",
            BlockExplorers("https://calibration.filfox.info/en/message"),
            EmptyIconUrls(),
            0,
            RpcEndpointsForURL("https://api.calibration.node.glif.io/rpc/v1"),
            "FIL",
            "Filecoin",
            18,
            coin,
            GetSupportedKeyringsForNetwork(coin, chain_id),
            DefaultProps()};
      }());
  return network_info->get();
}

// Precompiled networks available in native wallet.
const std::vector<const mojom::NetworkInfo*>& GetKnownEthNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetEthMainnet(),
      GetAuroraMainnet(),
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

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Solana Mainnet Beta",
                BlockExplorers("https://explorer.solana.com/"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForKnownChain(chain_id),
                "SOL",
                "Solana",
                9,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetSolTestnet() {
  const auto coin = mojom::CoinType::SOL;
  const auto* chain_id = mojom::kSolanaTestnet;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Solana Testnet",
                BlockExplorers("https://explorer.solana.com/?cluster=testnet"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForURL("https://api.testnet.solana.com"),
                "SOL",
                "Solana",
                9,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetSolDevnet() {
  const auto coin = mojom::CoinType::SOL;
  const auto* chain_id = mojom::kSolanaDevnet;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Solana Devnet",
                BlockExplorers("https://explorer.solana.com/?cluster=devnet"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForURL("https://api.devnet.solana.com"),
                "SOL",
                "Solana",
                9,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetSolLocalhost() {
  const auto coin = mojom::CoinType::SOL;
  const auto* chain_id = mojom::kLocalhostChainId;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Solana Localhost",
                BlockExplorers(
                    "https://explorer.solana.com/"
                    "?cluster=custom&customUrl=http%3A%2F%2Flocalhost%3A8899"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForURL(kSolanaLocalhostURL),
                "SOL",
                "Solana",
                9,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
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

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Filecoin Mainnet",
                BlockExplorers("https://filscan.io/tipset/message-detail"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForURL("https://api.node.glif.io/rpc/v0"),
                "FIL",
                "Filecoin",
                18,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetFilTestnet() {
  const auto coin = mojom::CoinType::FIL;
  const auto* chain_id = mojom::kFilecoinTestnet;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {
            std::in_place,
            chain_id,
            "Filecoin Testnet",
            BlockExplorers(
                "https://calibration.filscan.io/tipset/message-detail"),
            EmptyIconUrls(),
            0,
            RpcEndpointsForURL("https://api.calibration.node.glif.io/rpc/v0"),
            "FIL",
            "Filecoin",
            18,
            coin,
            GetSupportedKeyringsForNetwork(coin, chain_id),
            DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetFilLocalhost() {
  const auto coin = mojom::CoinType::FIL;
  const auto* chain_id = mojom::kLocalhostChainId;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Filecoin Localhost",
                BlockExplorers(kFilecoinLocalhostURL),
                EmptyIconUrls(),
                0,
                RpcEndpointsForURL(kFilecoinLocalhostURL),
                "FIL",
                "Filecoin",
                18,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
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
  return GURL("https://zec.rocks:443/");
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

const mojom::NetworkInfo* GetBitcoinMainnet() {
  const auto coin = mojom::CoinType::BTC;
  const auto* chain_id = mojom::kBitcoinMainnet;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Bitcoin Mainnet",
                BlockExplorers("https://www.blockchain.com/explorer"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForKnownChain(chain_id),
                "BTC",
                "Bitcoin",
                8,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetBitcoinTestnet() {
  const auto coin = mojom::CoinType::BTC;
  const auto* chain_id = mojom::kBitcoinTestnet;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Bitcoin Testnet",
                BlockExplorers("https://blockstream.info/testnet"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForURL(kBitcoinTestnetRpcEndpoint),
                "BTC",
                "Bitcoin",
                8,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetZCashMainnet() {
  const auto coin = mojom::CoinType::ZEC;
  const auto* chain_id = mojom::kZCashMainnet;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Zcash Mainnet",
                BlockExplorers("https://3xpl.com/zcash/transaction"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForURL(ZCashMainnetRpcUrl().spec()),
                "ZEC",
                "Zcash",
                8,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
}

const mojom::NetworkInfo* GetZCashTestnet() {
  const auto coin = mojom::CoinType::ZEC;
  const auto* chain_id = mojom::kZCashTestnet;

  static base::NoDestructor<mojom::NetworkInfoPtr> network_info(
      [&]() -> mojom::NetworkInfoPtr {
        return {std::in_place,
                chain_id,
                "Zcash Testnet",
                BlockExplorers("https://blockexplorer.one/zcash/testnet/tx"),
                EmptyIconUrls(),
                0,
                RpcEndpointsForURL(ZCashTestnetRpcUrl().spec()),
                "ZEC",
                "Zcash",
                8,
                coin,
                GetSupportedKeyringsForNetwork(coin, chain_id),
                DefaultProps()};
      }());
  return network_info->get();
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

const std::vector<const mojom::NetworkInfo*>& GetKnownBitcoinNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks({
      // clang-format off
      GetBitcoinMainnet(),
      GetBitcoinTestnet(),
      // clang-format on
  });
  return *networks.get();
}

const std::vector<const mojom::NetworkInfo*>& GetAllKnownNetworks() {
  static base::NoDestructor<std::vector<const mojom::NetworkInfo*>> networks(
      [] {
        std::vector<const mojom::NetworkInfo*> result;
        for (const auto* network : GetKnownEthNetworks()) {
          result.push_back(network);
        }
        for (const auto* network : GetKnownSolNetworks()) {
          result.push_back(network);
        }
        for (const auto* network : GetKnownFilNetworks()) {
          result.push_back(network);
        }
        for (const auto* network : GetKnownBitcoinNetworks()) {
          result.push_back(network);
        }
        for (const auto* network : GetKnownZCashNetworks()) {
          result.push_back(network);
        }
        return result;
      }());

  return *networks.get();
}

// DEPRECATED 01/2024.
const base::flat_map<std::string, std::string> kInfuraSubdomains = {
    {brave_wallet::mojom::kMainnetChainId, "mainnet"},
    {"0x5", "goerli"},
    {brave_wallet::mojom::kSepoliaChainId, "sepolia"}};

// DEPRECATED 01/2024. For migration only.
const base::flat_map<std::string, std::string> kSolanaSubdomains = {
    {brave_wallet::mojom::kSolanaMainnet, "mainnet"},
    {brave_wallet::mojom::kSolanaTestnet, "testnet"},
    {brave_wallet::mojom::kSolanaDevnet, "devnet"}};

// DEPRECATED 01/2024. For migration only.
const base::flat_map<std::string, std::string> kFilecoinSubdomains = {
    {brave_wallet::mojom::kFilecoinMainnet, "mainnet"},
    {brave_wallet::mojom::kFilecoinTestnet, "testnet"}};

// DEPRECATED 01/2024. For migration only.
const base::flat_map<std::string, std::string> kBitcoinSubdomains = {
    {mojom::kBitcoinMainnet, "mainnet"},
    {mojom::kBitcoinTestnet, "testnet"}};

// DEPRECATED 01/2024. For migration only.
const base::flat_map<std::string, std::string> kZCashSubdomains = {
    {mojom::kZCashMainnet, "mainnet"},
    {mojom::kZCashTestnet, "testnet"}};

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
  }
  NOTREACHED_IN_MIGRATION() << coin;
  return "";
}

std::vector<mojom::NetworkInfoPtr> GetAllCustomNetworks(PrefService* prefs) {
  std::vector<mojom::NetworkInfoPtr> result;

  const auto& custom_networks = prefs->GetDict(kBraveWalletCustomNetworks);

  for (auto coin : GetSupportedCoins()) {
    if (auto* custom_list =
            custom_networks.FindList(GetPrefKeyForCoinType(coin))) {
      for (const auto& it : *custom_list) {
        mojom::NetworkInfoPtr chain = ValueToNetworkInfo(it);
        if (chain) {
          DCHECK_EQ(chain->coin, coin);
          chain->props = mojom::NetworkProps::New();
          result.push_back(std::move(chain));
        }
      }
    }
  }

  return result;
}

std::set<NetworkManager::NetworkId> GetAllHiddenNetworks(PrefService* prefs) {
  std::set<NetworkManager::NetworkId> result;

  const auto& hidden_networks = prefs->GetDict(kBraveWalletHiddenNetworks);

  for (auto coin : GetSupportedCoins()) {
    if (auto* hidden_networks_list =
            hidden_networks.FindList(GetPrefKeyForCoinType(coin))) {
      for (const auto& it : *hidden_networks_list) {
        if (auto* chain_id = it.GetIfString()) {
          result.insert({coin, base::ToLowerASCII(*chain_id)});
        }
      }
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

std::vector<mojom::NetworkInfoPtr> MergeKnownAndCustomNetworks(
    PrefService* prefs) {
  std::vector<mojom::NetworkInfoPtr> result;

  auto custom_networks = GetAllCustomNetworks(prefs);
  // Put all known chains to result, replace with custom one if chain_id
  // matches.
  for (auto* known_network : GetAllKnownNetworks()) {
    bool add_known_chain = true;
    for (auto& custom_network : custom_networks) {
      if (custom_network && custom_network->coin == known_network->coin &&
          base::EqualsCaseInsensitiveASCII(custom_network->chain_id,
                                           known_network->chain_id)) {
        result.push_back(std::move(custom_network));
        add_known_chain = false;
        break;
      }
    }
    if (add_known_chain) {
      result.push_back(known_network->Clone());
    }

    result.back()->props->is_known = true;
    result.back()->props->is_custom = !add_known_chain;
  }

  // Put all remaining custom chains to result.
  for (auto& custom_network : custom_networks) {
    if (custom_network) {
      result.push_back(std::move(custom_network));
      result.back()->props->is_known = false;
      result.back()->props->is_custom = true;
    }
  }

  auto hidden_networks = GetAllHiddenNetworks(prefs);
  for (auto& network : result) {
    network->props->is_hidden =
        hidden_networks.contains({network->coin, network->chain_id});
  }

  auto eth_default = GetCurrentChainIdFromPrefs(prefs, mojom::CoinType::ETH);
  auto sol_default = GetCurrentChainIdFromPrefs(prefs, mojom::CoinType::SOL);
  for (auto& network : result) {
    network->props->is_dapp_default = (network->coin == mojom::CoinType::ETH &&
                                       network->chain_id == eth_default) ||
                                      (network->coin == mojom::CoinType::SOL &&
                                       network->chain_id == sol_default);
  }

  return result;
}

}  // namespace

NetworkManager::NetworkManager(PrefService* prefs) : prefs_(prefs) {
  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      kBraveWalletCustomNetworks,
      base::BindRepeating(&NetworkManager::InvalidateCache,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kBraveWalletHiddenNetworks,
      base::BindRepeating(&NetworkManager::InvalidateCache,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kBraveWalletSelectedNetworksPerOrigin,
      base::BindRepeating(&NetworkManager::InvalidateCache,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kBraveWalletSelectedNetworks,
      base::BindRepeating(&NetworkManager::InvalidateCache,
                          base::Unretained(this)));
}
NetworkManager::~NetworkManager() = default;

mojom::NetworkInfoPtr NetworkManager::GetChain(const std::string& chain_id,
                                               mojom::CoinType coin) {
  if (auto* network = FindNetworkInternal(coin, chain_id)) {
    return network->Clone();
  }
  return nullptr;
}

// DEPRECATED 04/2024
std::string GetInfuraSubdomainForKnownChainId(const std::string& chain_id) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (kInfuraSubdomains.contains(chain_id_lower)) {
    return kInfuraSubdomains.at(chain_id_lower);
  }
  return std::string();
}

// DEPRECATED 01/2024. For migration only.
std::string GetSolanaSubdomainForKnownChainId(const std::string& chain_id) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (kSolanaSubdomains.contains(chain_id_lower)) {
    return kSolanaSubdomains.at(chain_id_lower);
  }
  return std::string();
}

// DEPRECATED 01/2024. For migration only.
std::string GetFilecoinSubdomainForKnownChainId(const std::string& chain_id) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (kFilecoinSubdomains.contains(chain_id_lower)) {
    return kFilecoinSubdomains.at(chain_id_lower);
  }
  return std::string();
}

// DEPRECATED 01/2024. For migration only.
std::string GetBitcoinSubdomainForKnownChainId(const std::string& chain_id) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (kBitcoinSubdomains.contains(chain_id_lower)) {
    return kBitcoinSubdomains.at(chain_id_lower);
  }
  return std::string();
}

// DEPRECATED 01/2024. For migration only.
std::string GetZCashSubdomainForKnownChainId(const std::string& chain_id) {
  std::string chain_id_lower = base::ToLowerASCII(chain_id);
  if (kZCashSubdomains.contains(chain_id_lower)) {
    return kZCashSubdomains.at(chain_id_lower);
  }
  return std::string();
}

// static
GURL NetworkManager::GetUnstoppableDomainsRpcUrl(const std::string& chain_id) {
  if (chain_id == mojom::kMainnetChainId) {
    return GetEthMainnet()->rpc_endpoints.front();
  } else if (chain_id == mojom::kPolygonMainnetChainId) {
    return GetPolygonMainnet()->rpc_endpoints.front();
  }

  NOTREACHED_NORETURN();
}

// static
GURL NetworkManager::GetEnsRpcUrl() {
  return GetEthMainnet()->rpc_endpoints.front();
}

// static
GURL NetworkManager::GetSnsRpcUrl() {
  return GetSolMainnet()->rpc_endpoints.front();
}

// static
std::vector<const mojom::NetworkInfo*> NetworkManager::GetAllKnownChains() {
  return GetAllKnownNetworks();
}

GURL NetworkManager::GetNetworkURL(const std::string& chain_id,
                                   mojom::CoinType coin) {
  auto* network = FindNetworkInternal(coin, chain_id);
  if (!network) {
    return GURL();
  }
  return GetActiveEndpointUrl(*network->get());
}

GURL NetworkManager::GetNetworkURL(mojom::CoinType coin,
                                   const std::optional<url::Origin>& origin) {
  return GetNetworkURL(GetCurrentChainId(coin, origin), coin);
}

std::vector<mojom::NetworkInfoPtr> NetworkManager::GetAllChains() {
  std::vector<mojom::NetworkInfoPtr> result;
  for (auto& network : GetAllNetworksInternal()) {
    result.push_back(network.Clone());
  }
  return result;
}

// DEPRECATED 01/2024. For migration only.
std::string GetKnownEthNetworkId(const std::string& chain_id) {
  auto subdomain = GetInfuraSubdomainForKnownChainId(chain_id);
  if (!subdomain.empty()) {
    return subdomain;
  }

  // For known networks not in kInfuraSubdomains:
  //   localhost: Use the first RPC URL.
  //   other: Use chain ID like other custom networks.
  for (const auto* network : GetKnownEthNetworks()) {
    if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) != 0) {
      continue;
    }
    if (base::CompareCaseInsensitiveASCII(chain_id, mojom::kLocalhostChainId) ==
        0) {
      return network->rpc_endpoints.front().spec();
    }
    return base::ToLowerASCII(chain_id);
  }

  return "";
}

// DEPRECATED 01/2024. For migration only.
std::string GetKnownSolNetworkId(const std::string& chain_id) {
  auto subdomain = GetSolanaSubdomainForKnownChainId(chain_id);
  if (!subdomain.empty()) {
    return subdomain;
  }

  // Separate check for localhost in known networks as it is predefined but
  // does not have predefined subdomain.
  if (base::CompareCaseInsensitiveASCII(chain_id, mojom::kLocalhostChainId) ==
      0) {
    for (const auto* network : GetKnownSolNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return network->rpc_endpoints.front().spec();
      }
    }
  }

  return "";
}

// DEPRECATED 01/2024. For migration only.
std::string GetKnownFilNetworkId(const std::string& chain_id) {
  auto subdomain = GetFilecoinSubdomainForKnownChainId(chain_id);
  if (!subdomain.empty()) {
    return subdomain;
  }

  // Separate check for localhost in known networks as it is predefined but
  // does not have predefined subdomain.
  if (base::CompareCaseInsensitiveASCII(chain_id, mojom::kLocalhostChainId) ==
      0) {
    for (const auto* network : GetKnownFilNetworks()) {
      if (base::CompareCaseInsensitiveASCII(network->chain_id, chain_id) == 0) {
        return network->rpc_endpoints.front().spec();
      }
    }
  }

  return "";
}

// DEPRECATED 01/2024. For migration only.
std::string GetKnownBtcNetworkId(const std::string& chain_id) {
  auto subdomain = GetBitcoinSubdomainForKnownChainId(chain_id);
  if (!subdomain.empty()) {
    return subdomain;
  }

  return "";
}

// DEPRECATED 01/2024. For migration only.
std::string GetKnownZecNetworkId(const std::string& chain_id) {
  auto subdomain = GetZCashSubdomainForKnownChainId(chain_id);
  if (!subdomain.empty()) {
    return subdomain;
  }

  return "";
}

// DEPRECATED 01/2024. For migration only.
std::string GetKnownNetworkId(mojom::CoinType coin,
                              const std::string& chain_id) {
  if (coin == mojom::CoinType::ETH) {
    return GetKnownEthNetworkId(chain_id);
  }
  if (coin == mojom::CoinType::SOL) {
    return GetKnownSolNetworkId(chain_id);
  }
  if (coin == mojom::CoinType::FIL) {
    return GetKnownFilNetworkId(chain_id);
  }
  if (coin == mojom::CoinType::BTC) {
    return GetKnownBtcNetworkId(chain_id);
  }
  if (coin == mojom::CoinType::ZEC) {
    return GetKnownZecNetworkId(chain_id);
  }
  NOTREACHED_IN_MIGRATION() << coin;
  return "";
}

// DEPRECATED 01/2024. For migration only.
// static
std::string NetworkManager::GetNetworkId_DEPRECATED(
    mojom::CoinType coin,
    const std::string& chain_id) {
  if (chain_id.empty()) {
    return "";
  }

  std::string id = GetKnownNetworkId(coin, chain_id);
  if (!id.empty()) {
    return id;
  }

  if (coin == mojom::CoinType::ETH) {
    return chain_id;
  }

  return "";
}

// DEPRECATED 01/2024. For migration only.
// static
std::optional<std::string> NetworkManager::GetChainIdByNetworkId_DEPRECATED(
    const mojom::CoinType& coin,
    const std::string& network_id) {
  if (network_id.empty()) {
    return std::nullopt;
  }
  for (const auto& network : GetAllKnownChains()) {
    if (network->coin != coin) {
      continue;
    }
    if (network_id == GetNetworkId_DEPRECATED(coin, network->chain_id)) {
      return network->chain_id;
    }
  }
  return base::ToLowerASCII(network_id);
}

std::optional<bool> NetworkManager::IsEip1559Chain(

    const std::string& chain_id) {
  auto chain_id_lwr = base::ToLowerASCII(chain_id);
  if (auto is_eip_1559 = prefs_->GetDict(kBraveWalletEip1559CustomChains)
                             .FindBool(chain_id_lwr)) {
    return is_eip_1559.value();
  }
  return GetEip1559ForKnownChain(chain_id_lwr);
}

void NetworkManager::SetEip1559ForCustomChain(const std::string& chain_id,
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

void NetworkManager::RemoveCustomNetwork(const std::string& chain_id,
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

void NetworkManager::SetNetworkHidden(mojom::CoinType coin,
                                      const std::string& chain_id,
                                      bool hidden) {
  ScopedDictPrefUpdate update(prefs_, kBraveWalletHiddenNetworks);
  base::Value::List* list = update->EnsureList(GetPrefKeyForCoinType(coin));

  if (hidden) {
    std::string chain_id_lower = base::ToLowerASCII(chain_id);
    if (!base::Contains(*list, base::Value(chain_id_lower))) {
      list->Append(chain_id_lower);
    }
  } else {
    list->EraseIf([&](const base::Value& v) {
      auto* chain_id_string = v.GetIfString();
      return chain_id_string &&
             base::EqualsCaseInsensitiveASCII(*chain_id_string, chain_id);
    });
  }
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
  if (coin == mojom::CoinType::ETH) {
    return mojom::kMainnetChainId;
  } else if (coin == mojom::CoinType::SOL) {
    return mojom::kSolanaMainnet;
  } else if (coin == mojom::CoinType::FIL) {
    return mojom::kFilecoinMainnet;
  } else if (coin == mojom::CoinType::BTC) {
    return mojom::kBitcoinMainnet;
  } else if (coin == mojom::CoinType::ZEC) {
    return mojom::kZCashMainnet;
  }
  NOTREACHED_NORETURN() << coin;
}

bool NetworkManager::SetCurrentChainId(mojom::CoinType coin,
                                       const std::optional<url::Origin>& origin,
                                       const std::string& chain_id) {
  // We cannot switch to an unknown chain_id
  if (!FindNetworkInternal(coin, chain_id)) {
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

void NetworkManager::BuildCache() {
  cached_networks_ = MergeKnownAndCustomNetworks(prefs_);
  cached_networks_map_.clear();
  for (auto& network : cached_networks_) {
    cached_networks_map_[{network->coin, network->chain_id}] = network.Clone();
  }
  cache_valid_ = true;
}

void NetworkManager::InvalidateCache() {
  cache_valid_ = false;
  cached_networks_.clear();
  cached_networks_map_.clear();
}

std::vector<mojom::NetworkInfoPtr>& NetworkManager::GetAllNetworksInternal() {
  if (!cache_valid_) {
    BuildCache();
  }
  CHECK(cache_valid_);
  return cached_networks_;
}

mojom::NetworkInfoPtr* NetworkManager::FindNetworkInternal(
    mojom::CoinType coin,
    const std::string& chain_id) {
  auto& networks_map = GetAllNetworksMapInternal();
  auto it = networks_map.find({coin, chain_id});
  if (it == networks_map.end()) {
    return nullptr;
  }
  return &it->second;
}

std::map<NetworkManager::NetworkId, mojom::NetworkInfoPtr>&
NetworkManager::GetAllNetworksMapInternal() {
  if (!cache_valid_) {
    BuildCache();
  }
  CHECK(cache_valid_);
  return cached_networks_map_;
}

}  // namespace brave_wallet
