/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/network_manager.h"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/contains.h"
#include "base/containers/flat_set.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/values_test_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/test_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/origin.h"

using base::test::ParseJsonDict;
using testing::Contains;
using testing::ElementsAreArray;
using testing::Eq;
using testing::Gt;
using testing::IsEmpty;
using testing::Not;

namespace brave_wallet {

// DEPRECATED 01/2024. For migration only.
std::string GetSolanaSubdomainForKnownChainId(const std::string& chain_id);
std::string GetFilecoinSubdomainForKnownChainId(const std::string& chain_id);
std::string GetBitcoinSubdomainForKnownChainId(const std::string& chain_id);
std::string GetZCashSubdomainForKnownChainId(const std::string& chain_id);
std::string GetKnownEthNetworkId(const std::string& chain_id);
std::string GetKnownSolNetworkId(const std::string& chain_id);
std::string GetKnownFilNetworkId(const std::string& chain_id);
std::string GetKnownBtcNetworkId(const std::string& chain_id);
std::string GetKnownZecNetworkId(const std::string& chain_id);

class NetworkManagerUnitTest : public testing::Test {
 public:
  NetworkManagerUnitTest() = default;
  void SetUp() override {
    RegisterProfilePrefs(prefs_.registry());
    network_manager_ = std::make_unique<NetworkManager>(&prefs_);
  }

  PrefService* prefs() { return &prefs_; }
  NetworkManager* network_manager() { return network_manager_.get(); }

  std::vector<const mojom::NetworkInfo*> GetAllKnownChains(
      mojom::CoinType coin) {
    std::vector<const mojom::NetworkInfo*> result;
    for (auto* network : NetworkManager::GetAllKnownChains()) {
      if (network->coin == coin) {
        result.push_back(network);
      }
    }
    return result;
  }

  mojom::NetworkInfoPtr GetKnownChain(const std::string& chain_id,
                                      mojom::CoinType coin) {
    for (auto* network : NetworkManager::GetAllKnownChains()) {
      if (network->coin == coin && network->chain_id == chain_id) {
        return network->Clone();
      }
    }
    return nullptr;
  }

  std::vector<mojom::NetworkInfoPtr> GetAllCustomChains(mojom::CoinType coin) {
    std::vector<mojom::NetworkInfoPtr> result;
    for (auto& network : network_manager()->GetAllChains()) {
      if (network->coin == coin && network->props->is_custom) {
        result.push_back(network->Clone());
      }
    }
    return result;
  }

 protected:
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<NetworkManager> network_manager_;
};

TEST_F(NetworkManagerUnitTest, GetAllChains) {
  const base::test::ScopedFeatureList scoped_feature_list{
      features::kBraveWalletZCashFeature};

  std::set<std::string> hidden_defaults = {
      mojom::kSepoliaChainId,
      mojom::kLocalhostChainId,
      mojom::kFilecoinEthereumTestnetChainId,
      mojom::kFilecoinTestnet,
      mojom::kLocalhostChainId,
      mojom::kSolanaDevnet,
      mojom::kSolanaTestnet,
      mojom::kLocalhostChainId,
      mojom::kBitcoinTestnet,
      mojom::kZCashTestnet};

  EXPECT_EQ(network_manager()->GetAllChains().size(), 22u);
  for (auto& network : network_manager()->GetAllChains()) {
    SCOPED_TRACE(network->chain_id);
    EXPECT_THAT(network->chain_id, Not(IsEmpty()));
    EXPECT_THAT(network->chain_name, Not(IsEmpty()));
    EXPECT_THAT(network->block_explorer_urls, Not(IsEmpty()));
    EXPECT_THAT(network->icon_urls, IsEmpty());
    EXPECT_THAT(network->active_rpc_endpoint_index, Eq(0));
    EXPECT_THAT(network->rpc_endpoints, Not(IsEmpty()));
    EXPECT_TRUE(network->rpc_endpoints[0].is_valid());
    EXPECT_THAT(network->symbol, Not(IsEmpty()));
    EXPECT_THAT(network->symbol_name, Not(IsEmpty()));
    EXPECT_THAT(network->decimals, Gt(0));
    EXPECT_THAT(GetSupportedCoins(), Contains(network->coin));
    EXPECT_EQ(network->supported_keyrings,
              GetSupportedKeyringsForNetwork(network->coin, network->chain_id));

    EXPECT_TRUE(network->props->is_known);
    EXPECT_FALSE(network->props->is_custom);
    EXPECT_EQ(network->props->is_hidden,
              base::Contains(hidden_defaults, network->chain_id));

    if (network->chain_id == mojom::kMainnetChainId ||
        network->chain_id == mojom::kSolanaMainnet) {
      EXPECT_TRUE(network->props->is_dapp_default);
    } else {
      EXPECT_FALSE(network->props->is_dapp_default);
    }

    EXPECT_EQ(network_manager()->GetChain(network->chain_id, network->coin),
              network);
  }

  auto custom_eth_mainnet =
      network_manager()->GetChain(mojom::kMainnetChainId, mojom::CoinType::ETH);
  custom_eth_mainnet->decimals = 77;
  network_manager()->AddCustomNetwork(*custom_eth_mainnet);
  custom_eth_mainnet->props->is_custom = true;
  EXPECT_EQ(network_manager()->GetAllChains()[0], custom_eth_mainnet.Clone());

  auto custom_eth_unknown =
      network_manager()->GetChain(mojom::kMainnetChainId, mojom::CoinType::ETH);
  custom_eth_unknown->chain_id = "0x12345";
  custom_eth_unknown->decimals = 77;
  network_manager()->AddCustomNetwork(*custom_eth_unknown);
  EXPECT_EQ(network_manager()->GetAllChains().size(), 23u);
  custom_eth_unknown->props->is_known = false;
  custom_eth_unknown->props->is_custom = true;
  custom_eth_unknown->props->is_dapp_default = false;
  EXPECT_EQ(network_manager()->GetAllChains()[22], custom_eth_unknown.Clone());

  EXPECT_TRUE(AllCoinsTested());
}

TEST_F(NetworkManagerUnitTest, GetNetworkURLTest) {
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1();
  mojom::NetworkInfo chain2 = GetTestNetworkInfo2();

  network_manager()->AddCustomNetwork(chain1);
  network_manager()->AddCustomNetwork(chain2);

  for (const auto& chain : NetworkManager::GetAllKnownChains()) {
    GURL rpc_url(chain->rpc_endpoints.front());

    EXPECT_EQ(rpc_url,
              network_manager()->GetNetworkURL(chain->chain_id, chain->coin));
  }
  EXPECT_EQ(
      chain1.rpc_endpoints.front(),
      network_manager()->GetNetworkURL(chain1.chain_id, mojom::CoinType::ETH));
  EXPECT_EQ(
      chain2.rpc_endpoints.front(),
      network_manager()->GetNetworkURL(chain2.chain_id, mojom::CoinType::ETH));

  EXPECT_EQ(GURL("https://solana-mainnet.wallet.brave.com"),
            network_manager()->GetNetworkURL(mojom::kSolanaMainnet,
                                             mojom::CoinType::SOL));
  auto custom_sol_network =
      GetKnownChain(mojom::kSolanaMainnet, mojom::CoinType::SOL);
  custom_sol_network->rpc_endpoints.emplace_back("https://test-sol.com");
  custom_sol_network->active_rpc_endpoint_index = 1;
  network_manager()->AddCustomNetwork(*custom_sol_network);

  EXPECT_EQ(GURL("https://test-sol.com"),
            network_manager()->GetNetworkURL(mojom::kSolanaMainnet,
                                             mojom::CoinType::SOL));

  EXPECT_EQ(GURL("https://api.node.glif.io/rpc/v0"),
            network_manager()->GetNetworkURL(mojom::kFilecoinMainnet,
                                             mojom::CoinType::FIL));
  auto custom_fil_network =
      GetKnownChain(mojom::kFilecoinMainnet, mojom::CoinType::FIL);
  custom_fil_network->rpc_endpoints.emplace_back("https://test-fil.com");
  custom_fil_network->active_rpc_endpoint_index = 1;
  network_manager()->AddCustomNetwork(*custom_fil_network);

  EXPECT_EQ(GURL("https://test-fil.com"),
            network_manager()->GetNetworkURL(mojom::kFilecoinMainnet,
                                             mojom::CoinType::FIL));

  EXPECT_EQ(GURL("https://bitcoin-mainnet.wallet.brave.com/"),
            network_manager()->GetNetworkURL(mojom::kBitcoinMainnet,
                                             mojom::CoinType::BTC));
  auto custom_btc_network =
      GetKnownChain(mojom::kBitcoinMainnet, mojom::CoinType::BTC);
  custom_btc_network->rpc_endpoints.emplace_back("https://test-btc.com");
  custom_btc_network->active_rpc_endpoint_index = 1;
  network_manager()->AddCustomNetwork(*custom_btc_network);

  EXPECT_EQ(GURL("https://test-btc.com"),
            network_manager()->GetNetworkURL(mojom::kBitcoinMainnet,
                                             mojom::CoinType::BTC));

  EXPECT_EQ(GURL("https://zec.rocks:443/"),
            network_manager()->GetNetworkURL(mojom::kZCashMainnet,
                                             mojom::CoinType::ZEC));
  auto custom_zec_network =
      GetKnownChain(mojom::kZCashMainnet, mojom::CoinType::ZEC);
  custom_zec_network->rpc_endpoints.emplace_back("https://test-zec.com");
  custom_zec_network->active_rpc_endpoint_index = 1;
  network_manager()->AddCustomNetwork(*custom_zec_network);

  EXPECT_EQ(GURL("https://test-zec.com"),
            network_manager()->GetNetworkURL(mojom::kZCashMainnet,
                                             mojom::CoinType::ZEC));

  EXPECT_TRUE(AllCoinsTested());
}

TEST_F(NetworkManagerUnitTest, GetNetworkURLForKnownChains) {
  // GetNetworkURL for these known chains should resolve to brave subdomain.
  base::flat_set<std::string> known_chains = {
      brave_wallet::mojom::kMainnetChainId,
      brave_wallet::mojom::kPolygonMainnetChainId,
      brave_wallet::mojom::kBnbSmartChainMainnetChainId,
      brave_wallet::mojom::kOptimismMainnetChainId,
      brave_wallet::mojom::kAuroraMainnetChainId,
      brave_wallet::mojom::kAvalancheMainnetChainId,
      brave_wallet::mojom::kSepoliaChainId};

  for (const auto& chain : GetAllKnownChains(mojom::CoinType::ETH)) {
    auto network_url =
        network_manager()->GetNetworkURL(chain->chain_id, mojom::CoinType::ETH);
    EXPECT_EQ(base::EndsWith(network_url.host(), ".brave.com"),
              known_chains.contains(chain->chain_id));
  }
}

// DEPRECATED 01/2024. For migration only.
TEST_F(NetworkManagerUnitTest, GetSolanaSubdomainForKnownChainId) {
  for (const auto& chain : GetAllKnownChains(mojom::CoinType::SOL)) {
    auto subdomain = GetSolanaSubdomainForKnownChainId(chain->chain_id);
    bool expected = (chain->chain_id == brave_wallet::mojom::kLocalhostChainId);
    ASSERT_EQ(subdomain.empty(), expected);
  }
}

// DEPRECATED 01/2024. For migration only.
TEST_F(NetworkManagerUnitTest, GetFilecoinSubdomainForKnownChainId) {
  for (const auto& chain : GetAllKnownChains(mojom::CoinType::FIL)) {
    auto subdomain = GetFilecoinSubdomainForKnownChainId(chain->chain_id);
    bool expected = (chain->chain_id == brave_wallet::mojom::kLocalhostChainId);
    ASSERT_EQ(subdomain.empty(), expected);
  }
}

// DEPRECATED 01/2024. For migration only.
TEST_F(NetworkManagerUnitTest, GetBitcoinSubdomainForKnownChainId) {
  for (const auto& chain : GetAllKnownChains(mojom::CoinType::BTC)) {
    auto subdomain = GetBitcoinSubdomainForKnownChainId(chain->chain_id);
    ASSERT_FALSE(subdomain.empty());
  }
}

// DEPRECATED 01/2024. For migration only.
TEST_F(NetworkManagerUnitTest, GetZCashSubdomainForKnownChainId) {
  for (const auto& chain : GetAllKnownChains(mojom::CoinType::ZEC)) {
    auto subdomain = GetZCashSubdomainForKnownChainId(chain->chain_id);
    ASSERT_FALSE(subdomain.empty());
  }
}

TEST_F(NetworkManagerUnitTest, GetKnownChain) {
  const base::flat_set<std::string> non_eip1559_networks = {
      brave_wallet::mojom::kLocalhostChainId,
      brave_wallet::mojom::kBnbSmartChainMainnetChainId,
      brave_wallet::mojom::kAuroraMainnetChainId,
      brave_wallet::mojom::kNeonEVMMainnetChainId};

  auto known_chains = GetAllKnownChains(mojom::CoinType::ETH);
  ASSERT_FALSE(known_chains.empty());
  for (const auto& chain : known_chains) {
    auto network = GetKnownChain(chain->chain_id, mojom::CoinType::ETH);
    EXPECT_EQ(network->chain_id, chain->chain_id);
    EXPECT_EQ(network->chain_name, chain->chain_name);
    EXPECT_TRUE(GetActiveEndpointUrl(*network).is_valid());
    EXPECT_EQ(network->icon_urls, chain->icon_urls);
    EXPECT_EQ(network->block_explorer_urls, chain->block_explorer_urls);
    EXPECT_EQ(network->symbol, chain->symbol);
    EXPECT_EQ(network->decimals, chain->decimals);
    EXPECT_EQ(network->symbol_name, chain->symbol_name);
  }
}

TEST_F(NetworkManagerUnitTest, GetChain) {
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1("0x5566");
  mojom::NetworkInfo chain2 = GetTestNetworkInfo1("0x89");
  network_manager()->AddCustomNetwork(chain1);
  network_manager()->AddCustomNetwork(chain2);

  // Ethereum
  EXPECT_FALSE(network_manager()->GetChain("", mojom::CoinType::ETH));

  EXPECT_FALSE(network_manager()->GetChain("0x123", mojom::CoinType::ETH));
  EXPECT_EQ(network_manager()->GetChain("0x5566", mojom::CoinType::ETH),
            chain1.Clone());
  mojom::NetworkInfo eth_mainnet(
      "0x1", "Ethereum Mainnet", {"https://etherscan.io"}, {}, 0,
      {GURL("https://ethereum-mainnet.wallet.brave.com")}, "ETH", "Ethereum",
      18, mojom::CoinType::ETH, {mojom::KeyringId::kDefault},
      mojom::NetworkProps::New(true, false, false, true));
  EXPECT_EQ(network_manager()->GetChain("0x1", mojom::CoinType::ETH),
            eth_mainnet.Clone());

  chain2.props->is_known = true;
  EXPECT_EQ(*network_manager()->GetChain("0x89", mojom::CoinType::ETH), chain2);

  // Solana
  mojom::NetworkInfo sol_mainnet(
      brave_wallet::mojom::kSolanaMainnet, "Solana Mainnet Beta",
      {"https://explorer.solana.com/"}, {}, 0,
      {GURL("https://solana-mainnet.wallet.brave.com")}, "SOL", "Solana", 9,
      brave_wallet::mojom::CoinType::SOL, {mojom::KeyringId::kSolana},
      mojom::NetworkProps::New(true, false, false, true));
  EXPECT_FALSE(network_manager()->GetChain("0x123", mojom::CoinType::SOL));
  EXPECT_EQ(network_manager()->GetChain("0x65", mojom::CoinType::SOL),
            sol_mainnet.Clone());

  // Filecoin
  mojom::NetworkInfo fil_mainnet(
      brave_wallet::mojom::kFilecoinMainnet, "Filecoin Mainnet",
      {"https://filscan.io/tipset/message-detail"}, {}, 0,
      {GURL("https://api.node.glif.io/rpc/v0")}, "FIL", "Filecoin", 18,
      brave_wallet::mojom::CoinType::FIL, {mojom::KeyringId::kFilecoin},
      mojom::NetworkProps::New(true, false, false, false));
  EXPECT_FALSE(network_manager()->GetChain("0x123", mojom::CoinType::FIL));
  EXPECT_EQ(network_manager()->GetChain("f", mojom::CoinType::FIL),
            fil_mainnet.Clone());

  // Bitcoin
  mojom::NetworkInfo btc_mainnet(
      mojom::kBitcoinMainnet, "Bitcoin Mainnet",
      {"https://www.blockchain.com/explorer"}, {}, 0,
      {GURL("https://bitcoin-mainnet.wallet.brave.com/")}, "BTC", "Bitcoin", 8,
      mojom::CoinType::BTC,
      {mojom::KeyringId::kBitcoin84, mojom::KeyringId::kBitcoinImport},
      mojom::NetworkProps::New(true, false, false, false));
  EXPECT_FALSE(network_manager()->GetChain("0x123", mojom::CoinType::BTC));
  EXPECT_EQ(
      network_manager()->GetChain("bitcoin_mainnet", mojom::CoinType::BTC),
      btc_mainnet.Clone());

  // Zcash
  mojom::NetworkInfo zec_mainnet(
      mojom::kZCashMainnet, "Zcash Mainnet",
      {"https://3xpl.com/zcash/transaction"}, {}, 0,
      {GURL("https://zec.rocks:443/")}, "ZEC", "Zcash", 8, mojom::CoinType::ZEC,
      {mojom::KeyringId::kZCashMainnet},
      mojom::NetworkProps::New(true, false, false, false));
  EXPECT_FALSE(network_manager()->GetChain("0x123", mojom::CoinType::ZEC));
  EXPECT_EQ(network_manager()->GetChain("zcash_mainnet", mojom::CoinType::ZEC),
            zec_mainnet.Clone());

  EXPECT_TRUE(AllCoinsTested());
}

// DEPRECATED 01/2024. For migration only.
TEST_F(NetworkManagerUnitTest, GetKnownEthNetworkId) {
  EXPECT_EQ(GetKnownEthNetworkId(mojom::kLocalhostChainId),
            "http://localhost:7545/");
  EXPECT_EQ(GetKnownEthNetworkId(mojom::kMainnetChainId), "mainnet");
  EXPECT_EQ(GetKnownEthNetworkId(mojom::kSepoliaChainId), "sepolia");
}

// DEPRECATED 01/2024. For migration only.
TEST_F(NetworkManagerUnitTest, GetKnownSolNetworkId) {
  EXPECT_EQ(GetKnownSolNetworkId(mojom::kLocalhostChainId),
            "http://localhost:8899/");
  EXPECT_EQ(GetKnownSolNetworkId(mojom::kSolanaMainnet), "mainnet");
  EXPECT_EQ(GetKnownSolNetworkId(mojom::kSolanaTestnet), "testnet");
  EXPECT_EQ(GetKnownSolNetworkId(mojom::kSolanaDevnet), "devnet");
}

// DEPRECATED 01/2024. For migration only.
TEST_F(NetworkManagerUnitTest, GetKnownFilNetworkId) {
  EXPECT_EQ(GetKnownFilNetworkId(mojom::kLocalhostChainId),
            "http://localhost:1234/rpc/v0");
  EXPECT_EQ(GetKnownFilNetworkId(mojom::kFilecoinMainnet), "mainnet");
  EXPECT_EQ(GetKnownFilNetworkId(mojom::kFilecoinTestnet), "testnet");
}

// DEPRECATED 01/2024. For migration only.
TEST_F(NetworkManagerUnitTest, GetNetworkId) {
  ASSERT_TRUE(GetAllCustomChains(mojom::CoinType::ETH).empty());

  EXPECT_EQ(NetworkManager::GetNetworkId_DEPRECATED(mojom::CoinType::ETH,
                                                    mojom::kMainnetChainId),
            "mainnet");
  EXPECT_EQ(NetworkManager::GetNetworkId_DEPRECATED(mojom::CoinType::ETH,
                                                    mojom::kLocalhostChainId),
            "http://localhost:7545/");
  EXPECT_EQ(
      NetworkManager::GetNetworkId_DEPRECATED(mojom::CoinType::ETH, "chain_id"),
      "chain_id");
  EXPECT_EQ(NetworkManager::GetNetworkId_DEPRECATED(mojom::CoinType::ETH,
                                                    "chain_id2"),
            "chain_id2");
  EXPECT_EQ(NetworkManager::GetNetworkId_DEPRECATED(
                mojom::CoinType::ETH, mojom::kPolygonMainnetChainId),
            mojom::kPolygonMainnetChainId);
  EXPECT_EQ(NetworkManager::GetNetworkId_DEPRECATED(
                mojom::CoinType::ETH, mojom::kBnbSmartChainMainnetChainId),
            mojom::kBnbSmartChainMainnetChainId);

  EXPECT_EQ(NetworkManager::GetNetworkId_DEPRECATED(mojom::CoinType::SOL,
                                                    mojom::kSolanaMainnet),
            "mainnet");
  EXPECT_EQ(NetworkManager::GetNetworkId_DEPRECATED(mojom::CoinType::SOL,
                                                    mojom::kSolanaTestnet),
            "testnet");
  EXPECT_EQ(NetworkManager::GetNetworkId_DEPRECATED(mojom::CoinType::SOL,
                                                    mojom::kSolanaDevnet),
            "devnet");
}

TEST_F(NetworkManagerUnitTest, Eip1559Chain) {
  auto dict = [&] {
    return prefs()->GetDict(kBraveWalletEip1559CustomChains).Clone();
  };

  EXPECT_TRUE(dict().empty());

  // Values for known chains.
  std::map<std::string, bool> known_states = {
      {mojom::kMainnetChainId, true},
      {mojom::kPolygonMainnetChainId, true},
      {mojom::kAvalancheMainnetChainId, true},
      {mojom::kOptimismMainnetChainId, true},
      {mojom::kSepoliaChainId, true},
      {mojom::kFilecoinEthereumMainnetChainId, true},
      {mojom::kFilecoinEthereumTestnetChainId, true},
      {mojom::kBnbSmartChainMainnetChainId, false},
      {mojom::kAuroraMainnetChainId, false},
      {mojom::kNeonEVMMainnetChainId, false},
      {mojom::kLocalhostChainId, false}};
  for (auto& [chain_id, value] : known_states) {
    EXPECT_EQ(network_manager()->IsEip1559Chain(chain_id).value(), value);
  }

  // Custom chain.
  const std::string custom_chain_id = "0xa23123";
  EXPECT_FALSE(network_manager()->IsEip1559Chain(custom_chain_id));

  network_manager()->SetEip1559ForCustomChain(custom_chain_id, true);
  EXPECT_TRUE(*network_manager()->IsEip1559Chain(custom_chain_id));
  EXPECT_EQ(*dict().FindBool(custom_chain_id), true);

  network_manager()->SetEip1559ForCustomChain(
      base::ToUpperASCII(custom_chain_id), false);
  EXPECT_FALSE(
      *network_manager()->IsEip1559Chain(base::ToUpperASCII(custom_chain_id)));
  EXPECT_EQ(*dict().FindBool(custom_chain_id), false);

  network_manager()->SetEip1559ForCustomChain(custom_chain_id, std::nullopt);
  EXPECT_FALSE(network_manager()->IsEip1559Chain(custom_chain_id).has_value());
  EXPECT_EQ(dict().FindBool(custom_chain_id), std::nullopt);

  // Custom chain overriding known one.
  network_manager()->SetEip1559ForCustomChain(mojom::kPolygonMainnetChainId,
                                              false);
  EXPECT_FALSE(
      *network_manager()->IsEip1559Chain(mojom::kPolygonMainnetChainId));
  EXPECT_EQ(*dict().FindBool(mojom::kPolygonMainnetChainId), false);

  network_manager()->SetEip1559ForCustomChain(mojom::kPolygonMainnetChainId,
                                              std::nullopt);
  EXPECT_TRUE(network_manager()->IsEip1559Chain(mojom::kPolygonMainnetChainId));
  EXPECT_EQ(dict().FindBool(mojom::kPolygonMainnetChainId), std::nullopt);
}

TEST_F(NetworkManagerUnitTest, CustomNetworkMatchesKnownNetwork) {
  auto get_polygon_from_all = [&] {
    for (const auto& chain : network_manager()->GetAllChains()) {
      if (chain->coin == mojom::CoinType::ETH &&
          chain->chain_id == mojom::kPolygonMainnetChainId) {
        return chain.Clone();
      }
    }
    return mojom::NetworkInfoPtr();
  };

  // Known network by default.
  EXPECT_EQ(get_polygon_from_all()->chain_name, "Polygon Mainnet");
  EXPECT_EQ(
      network_manager()
          ->GetNetworkURL(mojom::kPolygonMainnetChainId, mojom::CoinType::ETH)
          .GetWithoutFilename(),
      GURL("https://polygon-mainnet.wallet.brave.com"));

  mojom::NetworkInfo chain1 =
      GetTestNetworkInfo1(mojom::kPolygonMainnetChainId);

  network_manager()->AddCustomNetwork(chain1);

  // Custom network overrides known one.
  EXPECT_EQ(get_polygon_from_all()->chain_name, "chain_name");
  EXPECT_EQ(
      network_manager()
          ->GetNetworkURL(mojom::kPolygonMainnetChainId, mojom::CoinType::ETH)
          .GetWithoutFilename(),
      GURL("https://url1.com/"));

  network_manager()->RemoveCustomNetwork(mojom::kPolygonMainnetChainId,
                                         mojom::CoinType::ETH);

  // Back to known when custom is removed.
  EXPECT_EQ(get_polygon_from_all()->chain_name, "Polygon Mainnet");
  EXPECT_EQ(
      network_manager()
          ->GetNetworkURL(mojom::kPolygonMainnetChainId, mojom::CoinType::ETH)
          .GetWithoutFilename(),
      GURL("https://polygon-mainnet.wallet.brave.com"));
}

TEST_F(NetworkManagerUnitTest, RemoveCustomNetwork) {
  mojom::NetworkInfo chain = GetTestNetworkInfo1();

  EXPECT_FALSE(
      network_manager()->GetChain(chain.chain_id, mojom::CoinType::ETH));

  network_manager()->AddCustomNetwork(chain);
  EXPECT_TRUE(network_manager()
                  ->GetChain(chain.chain_id, mojom::CoinType::ETH)
                  ->props->is_custom);

  network_manager()->RemoveCustomNetwork(chain.chain_id, mojom::CoinType::ETH);
  EXPECT_FALSE(
      network_manager()->GetChain(chain.chain_id, mojom::CoinType::ETH));

  // Should not crash.
  network_manager()->RemoveCustomNetwork("unknown network",
                                         mojom::CoinType::ETH);

  {
    mojom::NetworkInfo chain_fil =
        GetTestNetworkInfo1(mojom::kFilecoinMainnet, mojom::CoinType::FIL);
    network_manager()->AddCustomNetwork(chain_fil);
    ASSERT_EQ(1u, GetAllCustomChains(mojom::CoinType::FIL).size());
    network_manager()->RemoveCustomNetwork(mojom::kFilecoinMainnet,
                                           mojom::CoinType::FIL);
    ASSERT_EQ(0u, GetAllCustomChains(mojom::CoinType::FIL).size());
  }

  {
    mojom::NetworkInfo chain_sol =
        GetTestNetworkInfo1(mojom::kSolanaMainnet, mojom::CoinType::SOL);
    network_manager()->AddCustomNetwork(chain_sol);
    ASSERT_EQ(1u, GetAllCustomChains(mojom::CoinType::SOL).size());
    network_manager()->RemoveCustomNetwork(mojom::kSolanaMainnet,
                                           mojom::CoinType::SOL);
    ASSERT_EQ(0u, GetAllCustomChains(mojom::CoinType::SOL).size());
  }

  {
    mojom::NetworkInfo chain_btc =
        GetTestNetworkInfo1(mojom::kBitcoinMainnet, mojom::CoinType::BTC);
    network_manager()->AddCustomNetwork(chain_btc);
    ASSERT_EQ(1u, GetAllCustomChains(mojom::CoinType::BTC).size());
    network_manager()->RemoveCustomNetwork(mojom::kBitcoinMainnet,
                                           mojom::CoinType::BTC);
    ASSERT_EQ(0u, GetAllCustomChains(mojom::CoinType::BTC).size());
  }

  {
    mojom::NetworkInfo chain_zec =
        GetTestNetworkInfo1(mojom::kZCashMainnet, mojom::CoinType::ZEC);
    network_manager()->AddCustomNetwork(chain_zec);
    ASSERT_EQ(1u, GetAllCustomChains(mojom::CoinType::ZEC).size());
    network_manager()->RemoveCustomNetwork(mojom::kZCashMainnet,
                                           mojom::CoinType::ZEC);
    ASSERT_EQ(0u, GetAllCustomChains(mojom::CoinType::ZEC).size());
  }

  EXPECT_TRUE(AllCoinsTested());
}

TEST_F(NetworkManagerUnitTest, RemoveCustomNetworkRemovesEip1559) {
  mojom::NetworkInfo chain = GetTestNetworkInfo1();

  network_manager()->AddCustomNetwork(chain);

  EXPECT_FALSE(network_manager()->IsEip1559Chain(chain.chain_id).has_value());
  network_manager()->SetEip1559ForCustomChain(chain.chain_id, true);
  EXPECT_TRUE(*network_manager()->IsEip1559Chain(chain.chain_id));

  network_manager()->RemoveCustomNetwork(chain.chain_id, mojom::CoinType::ETH);
  EXPECT_FALSE(network_manager()->IsEip1559Chain(chain.chain_id).has_value());
}

TEST_F(NetworkManagerUnitTest, SetNetworkHidden) {
  std::map<mojom::CoinType, std::vector<std::string>> hidden_per_coin;
  for (auto& network : network_manager()->GetAllChains()) {
    if (network->props->is_hidden) {
      hidden_per_coin[network->coin].push_back(network->chain_id);
    }
  }

  EXPECT_THAT(hidden_per_coin[mojom::CoinType::ETH],
              ElementsAreArray<std::string>({
                  mojom::kSepoliaChainId,
                  mojom::kFilecoinEthereumTestnetChainId,
                  mojom::kLocalhostChainId,
              }));
  EXPECT_THAT(hidden_per_coin[mojom::CoinType::FIL],
              ElementsAreArray<std::string>(
                  {mojom::kFilecoinTestnet, mojom::kLocalhostChainId}));
  EXPECT_THAT(hidden_per_coin[mojom::CoinType::SOL],
              ElementsAreArray<std::string>({mojom::kSolanaTestnet,
                                             mojom::kSolanaDevnet,
                                             mojom::kLocalhostChainId}));
  EXPECT_THAT(hidden_per_coin[mojom::CoinType::BTC],
              ElementsAreArray<std::string>({mojom::kBitcoinTestnet}));
  EXPECT_THAT(hidden_per_coin[mojom::CoinType::ZEC],
              ElementsAreArray<std::string>({mojom::kZCashTestnet}));
  EXPECT_TRUE(AllCoinsTested());

  // Can show/hide visible by default.
  EXPECT_FALSE(network_manager()
                   ->GetChain(mojom::kSolanaMainnet, mojom::CoinType::SOL)
                   ->props->is_hidden);
  network_manager()->SetNetworkHidden(mojom::CoinType::SOL,
                                      mojom::kSolanaMainnet, true);
  EXPECT_TRUE(network_manager()
                  ->GetChain(mojom::kSolanaMainnet, mojom::CoinType::SOL)
                  ->props->is_hidden);
  network_manager()->SetNetworkHidden(mojom::CoinType::SOL,
                                      mojom::kSolanaMainnet, false);
  EXPECT_FALSE(network_manager()
                   ->GetChain(mojom::kSolanaMainnet, mojom::CoinType::SOL)
                   ->props->is_hidden);

  // Can show/hide hidden by default.
  EXPECT_TRUE(network_manager()
                  ->GetChain(mojom::kBitcoinTestnet, mojom::CoinType::BTC)
                  ->props->is_hidden);
  network_manager()->SetNetworkHidden(mojom::CoinType::BTC,
                                      mojom::kBitcoinTestnet, false);
  EXPECT_FALSE(network_manager()
                   ->GetChain(mojom::kBitcoinTestnet, mojom::CoinType::BTC)
                   ->props->is_hidden);
  network_manager()->SetNetworkHidden(mojom::CoinType::BTC,
                                      mojom::kBitcoinTestnet, true);
  EXPECT_TRUE(network_manager()
                  ->GetChain(mojom::kBitcoinTestnet, mojom::CoinType::BTC)
                  ->props->is_hidden);

  // Can show/hide custom.
  auto custom_eth = network_manager()->GetChain(mojom::kPolygonMainnetChainId,
                                                mojom::CoinType::ETH);
  custom_eth->chain_id = "0x12345";
  custom_eth->decimals = 77;
  network_manager()->AddCustomNetwork(*custom_eth);
  EXPECT_FALSE(network_manager()
                   ->GetChain("0x12345", mojom::CoinType::ETH)
                   ->props->is_hidden);
  network_manager()->SetNetworkHidden(mojom::CoinType::ETH, "0x12345", true);
  EXPECT_TRUE(network_manager()
                  ->GetChain("0x12345", mojom::CoinType::ETH)
                  ->props->is_hidden);
  network_manager()->SetNetworkHidden(mojom::CoinType::ETH, "0x12345", false);
  EXPECT_FALSE(network_manager()
                   ->GetChain("0x12345", mojom::CoinType::ETH)
                   ->props->is_hidden);
}

TEST_F(NetworkManagerUnitTest, GetAndSetCurrentChainId) {
  const base::flat_map<mojom::CoinType, std::string> default_chain_ids = {
      {mojom::CoinType::ETH, mojom::kMainnetChainId},
      {mojom::CoinType::SOL, mojom::kSolanaMainnet},
      {mojom::CoinType::FIL, mojom::kFilecoinMainnet},
  };
  const base::flat_map<mojom::CoinType, std::string> new_default_chain_ids = {
      {mojom::CoinType::ETH, mojom::kSepoliaChainId},
      {mojom::CoinType::SOL, mojom::kSolanaTestnet},
      {mojom::CoinType::FIL, mojom::kFilecoinTestnet},
  };

  EXPECT_TRUE(AllCoinsTested());

  for (const auto coin_type : kAllCoins) {
    // TODO(apaymyshev): make this test working for BTC which has no localhost
    if (coin_type == mojom::CoinType::BTC ||
        coin_type == mojom::CoinType::ZEC) {
      continue;
    }

    // default value
    EXPECT_EQ(network_manager()->GetCurrentChainId(coin_type, std::nullopt),
              default_chain_ids.at(coin_type));

    // fallback to default
    EXPECT_EQ(network_manager()->GetCurrentChainId(
                  coin_type, url::Origin::Create(GURL("https://a.com"))),
              default_chain_ids.at(coin_type));
    EXPECT_EQ(network_manager()->GetCurrentChainId(coin_type, url::Origin()),
              default_chain_ids.at(coin_type));
    EXPECT_EQ(network_manager()->GetCurrentChainId(
                  coin_type, url::Origin::Create(GURL("file:///etc/passwd"))),
              default_chain_ids.at(coin_type));

    // unknown chain_id
    EXPECT_FALSE(network_manager()->SetCurrentChainId(
        coin_type, url::Origin::Create(GURL("https://a.com")), "0x5566"));
    EXPECT_EQ(network_manager()->GetCurrentChainId(
                  coin_type, url::Origin::Create(GURL("https://a.com"))),
              default_chain_ids.at(coin_type));
    EXPECT_FALSE(network_manager()->SetCurrentChainId(coin_type, std::nullopt,
                                                      "0x5566"));
    EXPECT_EQ(network_manager()->GetCurrentChainId(coin_type, std::nullopt),
              default_chain_ids.at(coin_type));

    EXPECT_TRUE(network_manager()->SetCurrentChainId(
        coin_type, url::Origin::Create(GURL("https://a.com")),
        mojom::kLocalhostChainId));
    EXPECT_EQ(network_manager()->GetCurrentChainId(
                  coin_type, url::Origin::Create(GURL("https://a.com"))),
              mojom::kLocalhostChainId);
    // other origin still use default
    EXPECT_EQ(network_manager()->GetCurrentChainId(
                  coin_type, url::Origin::Create(GURL("https://b.com"))),
              default_chain_ids.at(coin_type));

    // opaque cannot change the default
    EXPECT_FALSE(network_manager()->SetCurrentChainId(
        coin_type, url::Origin(), mojom::kLocalhostChainId));
    EXPECT_FALSE(network_manager()->SetCurrentChainId(
        coin_type, url::Origin::Create(GURL("about:blank")),
        mojom::kLocalhostChainId));
    EXPECT_EQ(network_manager()->GetCurrentChainId(coin_type, std::nullopt),
              default_chain_ids.at(coin_type));

    // now we change the default
    EXPECT_TRUE(network_manager()->SetCurrentChainId(
        coin_type, std::nullopt, new_default_chain_ids.at(coin_type)));
    EXPECT_EQ(network_manager()->GetCurrentChainId(coin_type, std::nullopt),
              new_default_chain_ids.at(coin_type));
    // should not affect per origin pref
    EXPECT_EQ(network_manager()->GetCurrentChainId(
                  coin_type, url::Origin::Create(GURL("https://a.com"))),
              mojom::kLocalhostChainId);

    // non http/https scheme will change the default
    EXPECT_TRUE(network_manager()->SetCurrentChainId(
        coin_type, url::Origin::Create(GURL("file:///etc/passwd")),
        default_chain_ids.at(coin_type)));
    EXPECT_EQ(network_manager()->GetCurrentChainId(coin_type, std::nullopt),
              default_chain_ids.at(coin_type));
    EXPECT_TRUE(network_manager()->SetCurrentChainId(
        coin_type, url::Origin::Create(GURL("chrome://wallet")),
        new_default_chain_ids.at(coin_type)));
    EXPECT_EQ(network_manager()->GetCurrentChainId(coin_type, std::nullopt),
              new_default_chain_ids.at(coin_type));
  }
}

TEST_F(NetworkManagerUnitTest, GetCurrentChainIdFallback) {
  mojom::NetworkInfo chain = GetTestNetworkInfo1();
  auto some_origin = url::Origin::Create(GURL("https://a.com"));

  network_manager()->AddCustomNetwork(chain);
  network_manager()->SetCurrentChainId(mojom::CoinType::ETH, std::nullopt,
                                       chain.chain_id);
  EXPECT_EQ(
      network_manager()->GetCurrentChainId(mojom::CoinType::ETH, std::nullopt),
      chain.chain_id);
  EXPECT_EQ(
      network_manager()->GetCurrentChainId(mojom::CoinType::ETH, some_origin),
      chain.chain_id);

  network_manager()->RemoveCustomNetwork(chain.chain_id, mojom::CoinType::ETH);
  EXPECT_EQ(
      network_manager()->GetCurrentChainId(mojom::CoinType::ETH, std::nullopt),
      mojom::kMainnetChainId);
  EXPECT_EQ(
      network_manager()->GetCurrentChainId(mojom::CoinType::ETH, some_origin),
      mojom::kMainnetChainId);

  chain.chain_id = "another_id";
  network_manager()->AddCustomNetwork(chain);
  network_manager()->SetCurrentChainId(mojom::CoinType::ETH, some_origin,
                                       chain.chain_id);
  EXPECT_EQ(
      network_manager()->GetCurrentChainId(mojom::CoinType::ETH, std::nullopt),
      mojom::kMainnetChainId);
  EXPECT_EQ(
      network_manager()->GetCurrentChainId(mojom::CoinType::ETH, some_origin),
      chain.chain_id);

  network_manager()->RemoveCustomNetwork(chain.chain_id, mojom::CoinType::ETH);
  EXPECT_EQ(
      network_manager()->GetCurrentChainId(mojom::CoinType::ETH, std::nullopt),
      mojom::kMainnetChainId);
  EXPECT_EQ(
      network_manager()->GetCurrentChainId(mojom::CoinType::ETH, some_origin),
      mojom::kMainnetChainId);
}

TEST_F(NetworkManagerUnitTest, GetUnstoppableDomainsRpcUrl) {
  EXPECT_EQ(
      GURL("https://ethereum-mainnet.wallet.brave.com"),
      NetworkManager::GetUnstoppableDomainsRpcUrl(mojom::kMainnetChainId));
  EXPECT_EQ(GURL("https://polygon-mainnet.wallet.brave.com"),
            NetworkManager::GetUnstoppableDomainsRpcUrl(
                mojom::kPolygonMainnetChainId));
}

TEST_F(NetworkManagerUnitTest, GetEnsRpcUrl) {
  EXPECT_EQ(GURL("https://ethereum-mainnet.wallet.brave.com"),
            NetworkManager::GetEnsRpcUrl());
}

TEST_F(NetworkManagerUnitTest, GetSnsRpcUrl) {
  EXPECT_EQ(GURL("https://solana-mainnet.wallet.brave.com"),
            NetworkManager::GetSnsRpcUrl());
}

// DEPRECATED 01/2024. For migration only.
TEST_F(NetworkManagerUnitTest, GetChainIdByNetworkId) {
  network_manager()->AddCustomNetwork(
      GetTestNetworkInfo1("chain_id1", mojom::CoinType::ETH));

  for (const auto& chain : network_manager()->GetAllChains()) {
    const auto coin_type = chain->coin;
    std::string nid;
    if (chain->coin == mojom::CoinType::ETH) {
      nid = GetKnownEthNetworkId(chain->chain_id);
    }
    if (chain->coin == mojom::CoinType::SOL) {
      nid = GetKnownSolNetworkId(chain->chain_id);
    }
    if (chain->coin == mojom::CoinType::FIL) {
      nid = GetKnownFilNetworkId(chain->chain_id);
    }
    if (chain->coin == mojom::CoinType::BTC) {
      nid = GetKnownBtcNetworkId(chain->chain_id);
    }
    if (chain->coin == mojom::CoinType::ZEC) {
      nid = GetKnownZecNetworkId(chain->chain_id);
    }
    if (nid.empty()) {
      ASSERT_EQ(chain->coin, mojom::CoinType::ETH);
      nid = chain->chain_id;
    }
    auto chain_id =
        NetworkManager::GetChainIdByNetworkId_DEPRECATED(coin_type, nid);
    ASSERT_TRUE(chain_id.has_value());
    EXPECT_EQ(chain->chain_id, chain_id.value());
  }
}

}  // namespace brave_wallet
