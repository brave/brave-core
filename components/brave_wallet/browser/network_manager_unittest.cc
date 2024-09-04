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

 protected:
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<NetworkManager> network_manager_;
};

TEST_F(NetworkManagerUnitTest, GetAllCustomChainsTest) {
  for (auto coin : kAllCoins) {
    SCOPED_TRACE(coin);
    ASSERT_TRUE(network_manager()->GetAllCustomChains(coin).empty());
    mojom::NetworkInfo chain1 = GetTestNetworkInfo1("chain1", coin);
    mojom::NetworkInfo chain2 = GetTestNetworkInfo2("chain2", coin);
    network_manager()->AddCustomNetwork(chain1);
    network_manager()->AddCustomNetwork(chain2);

    ASSERT_EQ(2u, network_manager()->GetAllCustomChains(coin).size());
    EXPECT_EQ(chain1, *network_manager()->GetAllCustomChains(coin)[0]);
    EXPECT_EQ(chain2, *network_manager()->GetAllCustomChains(coin)[1]);
  }
  EXPECT_TRUE(AllCoinsTested());
}

TEST_F(NetworkManagerUnitTest, KnownChainExists) {
  mojom::NetworkInfo chain = GetTestNetworkInfo1();
  network_manager()->AddCustomNetwork(chain);

  auto known_chains = NetworkManager::GetAllKnownChains(mojom::CoinType::ETH);
  EXPECT_EQ(known_chains.size(), 11u);
  for (auto& known_chain : known_chains) {
    EXPECT_TRUE(network_manager()->KnownChainExists(known_chain->chain_id,
                                                    mojom::CoinType::ETH));
    // Test that uppercase chain ID works too
    EXPECT_TRUE(network_manager()->KnownChainExists(
        base::ToUpperASCII(known_chain->chain_id), mojom::CoinType::ETH));
  }

  EXPECT_TRUE(network_manager()->CustomChainExists(chain.chain_id,
                                                   mojom::CoinType::ETH));
  // Test that uppercase chain ID works too
  EXPECT_TRUE(network_manager()->CustomChainExists(
      base::ToUpperASCII(chain.chain_id), mojom::CoinType::ETH));
  EXPECT_FALSE(network_manager()->KnownChainExists(chain.chain_id,
                                                   mojom::CoinType::ETH));

  EXPECT_TRUE(network_manager()->KnownChainExists(mojom::kFilecoinMainnet,
                                                  mojom::CoinType::FIL));
  EXPECT_TRUE(network_manager()->KnownChainExists(mojom::kFilecoinTestnet,
                                                  mojom::CoinType::FIL));
  EXPECT_TRUE(network_manager()->KnownChainExists(mojom::kLocalhostChainId,
                                                  mojom::CoinType::FIL));

  EXPECT_TRUE(network_manager()->KnownChainExists(mojom::kSolanaMainnet,
                                                  mojom::CoinType::SOL));
  EXPECT_TRUE(network_manager()->KnownChainExists(mojom::kSolanaTestnet,
                                                  mojom::CoinType::SOL));
  EXPECT_TRUE(network_manager()->KnownChainExists(mojom::kSolanaDevnet,
                                                  mojom::CoinType::SOL));
  EXPECT_TRUE(network_manager()->KnownChainExists(mojom::kLocalhostChainId,
                                                  mojom::CoinType::SOL));

  EXPECT_TRUE(network_manager()->KnownChainExists(mojom::kBitcoinMainnet,
                                                  mojom::CoinType::BTC));
  EXPECT_TRUE(network_manager()->KnownChainExists(mojom::kBitcoinTestnet,
                                                  mojom::CoinType::BTC));

  EXPECT_TRUE(network_manager()->KnownChainExists(mojom::kZCashMainnet,
                                                  mojom::CoinType::ZEC));
  EXPECT_TRUE(network_manager()->KnownChainExists(mojom::kZCashTestnet,
                                                  mojom::CoinType::ZEC));

  EXPECT_TRUE(AllCoinsTested());
}

TEST_F(NetworkManagerUnitTest, CustomChainExists) {
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1();

  mojom::NetworkInfo chain2 = GetTestNetworkInfo2();

  EXPECT_FALSE(network_manager()->CustomChainExists(chain1.chain_id,
                                                    mojom::CoinType::ETH));
  EXPECT_FALSE(network_manager()->CustomChainExists(chain2.chain_id,
                                                    mojom::CoinType::ETH));
  EXPECT_EQ(network_manager()->GetAllCustomChains(mojom::CoinType::ETH).size(),
            0u);
  network_manager()->AddCustomNetwork(chain1);
  network_manager()->AddCustomNetwork(chain2);

  EXPECT_TRUE(network_manager()->CustomChainExists(chain1.chain_id,
                                                   mojom::CoinType::ETH));
  EXPECT_TRUE(network_manager()->CustomChainExists(chain2.chain_id,
                                                   mojom::CoinType::ETH));
  EXPECT_EQ(network_manager()->GetAllCustomChains(mojom::CoinType::ETH).size(),
            2u);

  EXPECT_FALSE(network_manager()->CustomChainExists(mojom::kFilecoinMainnet,
                                                    mojom::CoinType::FIL));
  network_manager()->AddCustomNetwork(
      *network_manager()->GetAllKnownChains(mojom::CoinType::FIL)[0]);
  EXPECT_TRUE(network_manager()->CustomChainExists(mojom::kFilecoinMainnet,
                                                   mojom::CoinType::FIL));

  EXPECT_FALSE(network_manager()->CustomChainExists(mojom::kSolanaMainnet,
                                                    mojom::CoinType::SOL));
  network_manager()->AddCustomNetwork(
      *network_manager()->GetAllKnownChains(mojom::CoinType::SOL)[0]);
  EXPECT_TRUE(network_manager()->CustomChainExists(mojom::kSolanaMainnet,
                                                   mojom::CoinType::SOL));

  EXPECT_FALSE(network_manager()->CustomChainExists(mojom::kBitcoinMainnet,
                                                    mojom::CoinType::BTC));
  network_manager()->AddCustomNetwork(
      *network_manager()->GetAllKnownChains(mojom::CoinType::BTC)[0]);
  EXPECT_TRUE(network_manager()->CustomChainExists(mojom::kBitcoinMainnet,
                                                   mojom::CoinType::BTC));

  EXPECT_FALSE(network_manager()->CustomChainExists(mojom::kZCashMainnet,
                                                    mojom::CoinType::ZEC));
  network_manager()->AddCustomNetwork(
      *network_manager()->GetAllKnownChains(mojom::CoinType::ZEC)[0]);
  EXPECT_TRUE(network_manager()->CustomChainExists(mojom::kZCashMainnet,
                                                   mojom::CoinType::ZEC));

  EXPECT_TRUE(AllCoinsTested());
}

TEST_F(NetworkManagerUnitTest, CustomChainsExist) {
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1();
  mojom::NetworkInfo chain2 = GetTestNetworkInfo2();

  std::vector<std::string> chains{chain1.chain_id, chain2.chain_id,
                                  "unexpected_chain"};
  // Before updating custom networks, none of the chains should be returned
  EXPECT_EQ(
      network_manager()->CustomChainsExist(chains, mojom::CoinType::ETH).size(),
      0u);
  network_manager()->AddCustomNetwork(chain1);
  network_manager()->AddCustomNetwork(chain2);

  // After updating custom networks the, only the added chains should be
  // returned
  std::vector<std::string> existing_chains =
      network_manager()->CustomChainsExist(chains, mojom::CoinType::ETH);
  EXPECT_EQ(existing_chains.size(), 2u);
  EXPECT_EQ(existing_chains[0], chain1.chain_id);
  EXPECT_EQ(existing_chains[1], chain2.chain_id);
  EXPECT_EQ(std::find(existing_chains.begin(), existing_chains.end(),
                      "unexpected_chain"),
            existing_chains.end());
}

TEST_F(NetworkManagerUnitTest, GetAllChainsTest) {
  const base::test::ScopedFeatureList scoped_feature_list{
      features::kBraveWalletZCashFeature};

  EXPECT_EQ(network_manager()->GetAllChains().size(), 22u);
  for (auto& chain : network_manager()->GetAllChains()) {
    EXPECT_TRUE(chain->rpc_endpoints[0].is_valid());
    EXPECT_EQ(chain->active_rpc_endpoint_index, 0);
  }

  auto get_all_chains_for_coin = [&](mojom::CoinType coin) {
    std::vector<mojom::NetworkInfoPtr> result;
    for (auto& chain : network_manager()->GetAllChains()) {
      if (chain->coin == coin) {
        result.push_back(std::move(chain));
      }
    }
    return result;
  };

  mojom::NetworkInfo chain1 =
      GetTestNetworkInfo1(mojom::kPolygonMainnetChainId);
  mojom::NetworkInfo chain2 = GetTestNetworkInfo2();
  network_manager()->AddCustomNetwork(chain1);
  network_manager()->AddCustomNetwork(chain2);

  auto known_chains = NetworkManager::GetAllKnownChains(mojom::CoinType::ETH);
  auto custom_chains =
      network_manager()->GetAllCustomChains(mojom::CoinType::ETH);
  EXPECT_EQ(*custom_chains[0], chain1);
  EXPECT_EQ(*custom_chains[1], chain2);

  // Custom Polygon chain takes place of known one.
  // Custom unknown chain becomes last.
  auto expected_chains = std::move(known_chains);
  EXPECT_EQ(expected_chains[2]->chain_id, mojom::kPolygonMainnetChainId);
  expected_chains[2] = chain1.Clone();
  expected_chains.push_back(chain2.Clone());

  auto all_chains = get_all_chains_for_coin(mojom::CoinType::ETH);

  EXPECT_EQ(expected_chains.size(), all_chains.size());
  for (size_t i = 0; i < all_chains.size(); i++) {
    ASSERT_TRUE(all_chains.at(i).Equals(expected_chains.at(i)));
    EXPECT_THAT(all_chains.at(i)->supported_keyrings,
                ElementsAreArray({mojom::KeyringId::kDefault}));
  }

  // Solana
  auto sol_main_custom =
      *network_manager()->GetAllKnownChains(mojom::CoinType::SOL)[0];
  sol_main_custom.decimals = 123;
  network_manager()->AddCustomNetwork(sol_main_custom);

  auto sol_chains = get_all_chains_for_coin(mojom::CoinType::SOL);
  ASSERT_EQ(sol_chains.size(), 4u);
  EXPECT_EQ(sol_chains[0]->chain_id, mojom::kSolanaMainnet);
  EXPECT_EQ(sol_chains[0]->decimals, 123);
  EXPECT_EQ(sol_chains[1]->chain_id, mojom::kSolanaTestnet);
  EXPECT_EQ(sol_chains[2]->chain_id, mojom::kSolanaDevnet);
  EXPECT_EQ(sol_chains[3]->chain_id, mojom::kLocalhostChainId);

  for (auto& sol_chain : sol_chains) {
    EXPECT_THAT(sol_chain->supported_keyrings,
                ElementsAreArray({mojom::KeyringId::kSolana}));
  }

  // Filecoin
  auto fil_main_custom =
      *network_manager()->GetAllKnownChains(mojom::CoinType::FIL)[0];
  fil_main_custom.decimals = 123;
  network_manager()->AddCustomNetwork(fil_main_custom);

  auto fil_chains = get_all_chains_for_coin(mojom::CoinType::FIL);
  ASSERT_EQ(fil_chains.size(), 3u);
  EXPECT_EQ(fil_chains[0]->chain_id, mojom::kFilecoinMainnet);
  EXPECT_EQ(fil_chains[0]->decimals, 123);
  EXPECT_EQ(fil_chains[1]->chain_id, mojom::kFilecoinTestnet);
  EXPECT_EQ(fil_chains[2]->chain_id, mojom::kLocalhostChainId);
  EXPECT_THAT(fil_chains[0]->supported_keyrings,
              ElementsAreArray({mojom::KeyringId::kFilecoin}));
  EXPECT_THAT(fil_chains[1]->supported_keyrings,
              ElementsAreArray({mojom::KeyringId::kFilecoinTestnet}));
  EXPECT_THAT(fil_chains[2]->supported_keyrings,
              ElementsAreArray({mojom::KeyringId::kFilecoinTestnet}));

  // Bitcoin
  auto btc_main_custom =
      *network_manager()->GetAllKnownChains(mojom::CoinType::BTC)[0];
  btc_main_custom.decimals = 123;
  network_manager()->AddCustomNetwork(btc_main_custom);

  auto btc_chains = get_all_chains_for_coin(mojom::CoinType::BTC);
  ASSERT_EQ(btc_chains.size(), 2u);
  EXPECT_EQ(btc_chains[0]->chain_id, mojom::kBitcoinMainnet);
  EXPECT_EQ(btc_chains[0]->decimals, 123);
  EXPECT_EQ(btc_chains[1]->chain_id, mojom::kBitcoinTestnet);
  EXPECT_THAT(btc_chains[0]->supported_keyrings,
              ElementsAreArray({mojom::KeyringId::kBitcoin84,
                                mojom::KeyringId::kBitcoinImport,
                                mojom::KeyringId::kBitcoinHardware}));
  EXPECT_THAT(btc_chains[1]->supported_keyrings,
              ElementsAreArray({mojom::KeyringId::kBitcoin84Testnet,
                                mojom::KeyringId::kBitcoinImportTestnet,
                                mojom::KeyringId::kBitcoinHardwareTestnet}));

  // ZCash
  auto zec_main_custom =
      *network_manager()->GetAllKnownChains(mojom::CoinType::ZEC)[0];
  zec_main_custom.decimals = 123;
  network_manager()->AddCustomNetwork(zec_main_custom);

  auto zec_chains = get_all_chains_for_coin(mojom::CoinType::ZEC);
  ASSERT_EQ(zec_chains.size(), 2u);
  EXPECT_EQ(zec_chains[0]->chain_id, mojom::kZCashMainnet);
  EXPECT_EQ(zec_chains[0]->decimals, 123);
  EXPECT_EQ(zec_chains[1]->chain_id, mojom::kZCashTestnet);
  EXPECT_THAT(zec_chains[0]->supported_keyrings,
              ElementsAreArray({mojom::KeyringId::kZCashMainnet}));
  EXPECT_THAT(zec_chains[1]->supported_keyrings,
              ElementsAreArray({mojom::KeyringId::kZCashTestnet}));

  EXPECT_TRUE(AllCoinsTested());
  EXPECT_TRUE(AllKeyringsTested());
}

TEST_F(NetworkManagerUnitTest, GetNetworkURLTest) {
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1();
  mojom::NetworkInfo chain2 = GetTestNetworkInfo2();

  network_manager()->AddCustomNetwork(chain1);
  network_manager()->AddCustomNetwork(chain2);

  for (const auto& chain :
       NetworkManager::GetAllKnownChains(mojom::CoinType::ETH)) {
    // Brave proxies should have infura key added to path.
    GURL rpc_url(chain->rpc_endpoints.front());

    EXPECT_EQ(rpc_url, network_manager()->GetNetworkURL(chain->chain_id,
                                                        mojom::CoinType::ETH));
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
  auto custom_sol_network = network_manager()->GetKnownChain(
      mojom::kSolanaMainnet, mojom::CoinType::SOL);
  custom_sol_network->rpc_endpoints.emplace_back("https://test-sol.com");
  custom_sol_network->active_rpc_endpoint_index = 1;
  network_manager()->AddCustomNetwork(*custom_sol_network);

  EXPECT_EQ(GURL("https://test-sol.com"),
            network_manager()->GetNetworkURL(mojom::kSolanaMainnet,
                                             mojom::CoinType::SOL));

  EXPECT_EQ(GURL("https://api.node.glif.io/rpc/v0"),
            network_manager()->GetNetworkURL(mojom::kFilecoinMainnet,
                                             mojom::CoinType::FIL));
  auto custom_fil_network = network_manager()->GetKnownChain(
      mojom::kFilecoinMainnet, mojom::CoinType::FIL);
  custom_fil_network->rpc_endpoints.emplace_back("https://test-fil.com");
  custom_fil_network->active_rpc_endpoint_index = 1;
  network_manager()->AddCustomNetwork(*custom_fil_network);

  EXPECT_EQ(GURL("https://test-fil.com"),
            network_manager()->GetNetworkURL(mojom::kFilecoinMainnet,
                                             mojom::CoinType::FIL));

  EXPECT_EQ(GURL("https://bitcoin-mainnet.wallet.brave.com/"),
            network_manager()->GetNetworkURL(mojom::kBitcoinMainnet,
                                             mojom::CoinType::BTC));
  auto custom_btc_network = network_manager()->GetKnownChain(
      mojom::kBitcoinMainnet, mojom::CoinType::BTC);
  custom_btc_network->rpc_endpoints.emplace_back("https://test-btc.com");
  custom_btc_network->active_rpc_endpoint_index = 1;
  network_manager()->AddCustomNetwork(*custom_btc_network);

  EXPECT_EQ(GURL("https://test-btc.com"),
            network_manager()->GetNetworkURL(mojom::kBitcoinMainnet,
                                             mojom::CoinType::BTC));

  EXPECT_EQ(GURL("https://zec.rocks:443/"),
            network_manager()->GetNetworkURL(mojom::kZCashMainnet,
                                             mojom::CoinType::ZEC));
  auto custom_zec_network = network_manager()->GetKnownChain(
      mojom::kZCashMainnet, mojom::CoinType::ZEC);
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

  for (const auto& chain :
       NetworkManager::GetAllKnownChains(mojom::CoinType::ETH)) {
    auto network_url =
        network_manager()->GetNetworkURL(chain->chain_id, mojom::CoinType::ETH);
    EXPECT_EQ(base::EndsWith(network_url.host(), ".brave.com"),
              known_chains.contains(chain->chain_id));
  }
}

// DEPRECATED 01/2024. For migration only.
TEST_F(NetworkManagerUnitTest, GetSolanaSubdomainForKnownChainId) {
  for (const auto& chain :
       NetworkManager::GetAllKnownChains(mojom::CoinType::SOL)) {
    auto subdomain = GetSolanaSubdomainForKnownChainId(chain->chain_id);
    bool expected = (chain->chain_id == brave_wallet::mojom::kLocalhostChainId);
    ASSERT_EQ(subdomain.empty(), expected);
  }
}

// DEPRECATED 01/2024. For migration only.
TEST_F(NetworkManagerUnitTest, GetFilecoinSubdomainForKnownChainId) {
  for (const auto& chain :
       NetworkManager::GetAllKnownChains(mojom::CoinType::FIL)) {
    auto subdomain = GetFilecoinSubdomainForKnownChainId(chain->chain_id);
    bool expected = (chain->chain_id == brave_wallet::mojom::kLocalhostChainId);
    ASSERT_EQ(subdomain.empty(), expected);
  }
}

// DEPRECATED 01/2024. For migration only.
TEST_F(NetworkManagerUnitTest, GetBitcoinSubdomainForKnownChainId) {
  for (const auto& chain :
       NetworkManager::GetAllKnownChains(mojom::CoinType::BTC)) {
    auto subdomain = GetBitcoinSubdomainForKnownChainId(chain->chain_id);
    ASSERT_FALSE(subdomain.empty());
  }
}

// DEPRECATED 01/2024. For migration only.
TEST_F(NetworkManagerUnitTest, GetZCashSubdomainForKnownChainId) {
  for (const auto& chain :
       NetworkManager::GetAllKnownChains(mojom::CoinType::ZEC)) {
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

  auto known_chains = NetworkManager::GetAllKnownChains(mojom::CoinType::ETH);
  ASSERT_FALSE(known_chains.empty());
  for (const auto& chain : known_chains) {
    auto network =
        network_manager()->GetKnownChain(chain->chain_id, mojom::CoinType::ETH);
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

TEST_F(NetworkManagerUnitTest, GetCustomChain) {
  EXPECT_FALSE(
      network_manager()->GetCustomChain("chain_id", mojom::CoinType::ETH));

  mojom::NetworkInfo chain = GetTestNetworkInfo1();
  network_manager()->AddCustomNetwork(chain);

  auto network =
      network_manager()->GetCustomChain(chain.chain_id, mojom::CoinType::ETH);
  ASSERT_TRUE(network);
  EXPECT_EQ(*network, chain);

  // Test that uppercase chain ID works too
  network = network_manager()->GetCustomChain(
      base::ToUpperASCII(chain.chain_id), mojom::CoinType::ETH);
  ASSERT_TRUE(network);
  EXPECT_EQ(*network, chain);
}

TEST_F(NetworkManagerUnitTest, GetChain) {
  mojom::NetworkInfo chain1 = GetTestNetworkInfo1("0x5566");
  mojom::NetworkInfo chain2 = GetTestNetworkInfo1("0x89");
  network_manager()->AddCustomNetwork(chain1);
  network_manager()->AddCustomNetwork(chain2);

  EXPECT_FALSE(network_manager()->GetChain("", mojom::CoinType::ETH));

  EXPECT_FALSE(network_manager()->GetChain("0x123", mojom::CoinType::ETH));
  EXPECT_EQ(network_manager()->GetChain("0x5566", mojom::CoinType::ETH),
            chain1.Clone());
  EXPECT_EQ(network_manager()->GetChain("0x1", mojom::CoinType::ETH),
            network_manager()->GetKnownChain("0x1", mojom::CoinType::ETH));
  EXPECT_EQ(network_manager()->GetChain("0x539", mojom::CoinType::ETH),
            network_manager()->GetKnownChain("0x539", mojom::CoinType::ETH));

  EXPECT_EQ(*network_manager()->GetChain("0x89", mojom::CoinType::ETH), chain2);

  // Solana
  mojom::NetworkInfo sol_mainnet(
      brave_wallet::mojom::kSolanaMainnet, "Solana Mainnet Beta",
      {"https://explorer.solana.com/"}, {}, 0,
      {GURL("https://solana-mainnet.wallet.brave.com")}, "SOL", "Solana", 9,
      brave_wallet::mojom::CoinType::SOL, {mojom::KeyringId::kSolana});
  EXPECT_FALSE(network_manager()->GetChain("0x123", mojom::CoinType::SOL));
  EXPECT_EQ(network_manager()->GetChain("0x65", mojom::CoinType::SOL),
            sol_mainnet.Clone());

  // Filecoin
  mojom::NetworkInfo fil_mainnet(
      brave_wallet::mojom::kFilecoinMainnet, "Filecoin Mainnet",
      {"https://filscan.io/tipset/message-detail"}, {}, 0,
      {GURL("https://api.node.glif.io/rpc/v0")}, "FIL", "Filecoin", 18,
      brave_wallet::mojom::CoinType::FIL, {mojom::KeyringId::kFilecoin});
  EXPECT_FALSE(network_manager()->GetChain("0x123", mojom::CoinType::FIL));
  EXPECT_EQ(network_manager()->GetChain("f", mojom::CoinType::FIL),
            fil_mainnet.Clone());

  // Bitcoin
  mojom::NetworkInfo btc_mainnet(
      mojom::kBitcoinMainnet, "Bitcoin Mainnet",
      {"https://www.blockchain.com/explorer"}, {}, 0,
      {GURL("https://bitcoin-mainnet.wallet.brave.com/")}, "BTC", "Bitcoin", 8,
      mojom::CoinType::BTC,
      {mojom::KeyringId::kBitcoin84, mojom::KeyringId::kBitcoinImport,
       mojom::KeyringId::kBitcoinHardware});
  EXPECT_FALSE(network_manager()->GetChain("0x123", mojom::CoinType::BTC));
  EXPECT_EQ(
      network_manager()->GetChain("bitcoin_mainnet", mojom::CoinType::BTC),
      btc_mainnet.Clone());

  // Zcash
  mojom::NetworkInfo zec_mainnet(mojom::kZCashMainnet, "Zcash Mainnet",
                                 {"https://3xpl.com/zcash/transaction"}, {}, 0,
                                 {GURL("https://zec.rocks:443/")}, "ZEC",
                                 "Zcash", 8, mojom::CoinType::ZEC,
                                 {mojom::KeyringId::kZCashMainnet});
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
  ASSERT_TRUE(
      network_manager()->GetAllCustomChains(mojom::CoinType::ETH).empty());

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

  network_manager()->AddCustomNetwork(chain);
  EXPECT_TRUE(network_manager()->CustomChainExists(chain.chain_id,
                                                   mojom::CoinType::ETH));

  network_manager()->RemoveCustomNetwork(chain.chain_id, mojom::CoinType::ETH);
  EXPECT_FALSE(network_manager()->CustomChainExists(chain.chain_id,
                                                    mojom::CoinType::ETH));

  // Should not crash.
  network_manager()->RemoveCustomNetwork("unknown network",
                                         mojom::CoinType::ETH);

  {
    mojom::NetworkInfo chain_fil =
        GetTestNetworkInfo1(mojom::kFilecoinMainnet, mojom::CoinType::FIL);
    network_manager()->AddCustomNetwork(chain_fil);
    ASSERT_EQ(
        1u, network_manager()->GetAllCustomChains(mojom::CoinType::FIL).size());
    network_manager()->RemoveCustomNetwork(mojom::kFilecoinMainnet,
                                           mojom::CoinType::FIL);
    ASSERT_EQ(
        0u, network_manager()->GetAllCustomChains(mojom::CoinType::FIL).size());
  }

  {
    mojom::NetworkInfo chain_sol =
        GetTestNetworkInfo1(mojom::kSolanaMainnet, mojom::CoinType::SOL);
    network_manager()->AddCustomNetwork(chain_sol);
    ASSERT_EQ(
        1u, network_manager()->GetAllCustomChains(mojom::CoinType::SOL).size());
    network_manager()->RemoveCustomNetwork(mojom::kSolanaMainnet,
                                           mojom::CoinType::SOL);
    ASSERT_EQ(
        0u, network_manager()->GetAllCustomChains(mojom::CoinType::SOL).size());
  }

  {
    mojom::NetworkInfo chain_btc =
        GetTestNetworkInfo1(mojom::kBitcoinMainnet, mojom::CoinType::BTC);
    network_manager()->AddCustomNetwork(chain_btc);
    ASSERT_EQ(
        1u, network_manager()->GetAllCustomChains(mojom::CoinType::BTC).size());
    network_manager()->RemoveCustomNetwork(mojom::kBitcoinMainnet,
                                           mojom::CoinType::BTC);
    ASSERT_EQ(
        0u, network_manager()->GetAllCustomChains(mojom::CoinType::BTC).size());
  }

  {
    mojom::NetworkInfo chain_zec =
        GetTestNetworkInfo1(mojom::kZCashMainnet, mojom::CoinType::ZEC);
    network_manager()->AddCustomNetwork(chain_zec);
    ASSERT_EQ(
        1u, network_manager()->GetAllCustomChains(mojom::CoinType::ZEC).size());
    network_manager()->RemoveCustomNetwork(mojom::kZCashMainnet,
                                           mojom::CoinType::ZEC);
    ASSERT_EQ(
        0u, network_manager()->GetAllCustomChains(mojom::CoinType::ZEC).size());
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

TEST_F(NetworkManagerUnitTest, HiddenNetworks) {
  EXPECT_THAT(network_manager()->GetHiddenNetworks(mojom::CoinType::ETH),
              ElementsAreArray<std::string>(
                  {mojom::kSepoliaChainId, mojom::kLocalhostChainId,
                   mojom::kFilecoinEthereumTestnetChainId}));
  EXPECT_THAT(network_manager()->GetHiddenNetworks(mojom::CoinType::FIL),
              ElementsAreArray<std::string>(
                  {mojom::kFilecoinTestnet, mojom::kLocalhostChainId}));
  EXPECT_THAT(network_manager()->GetHiddenNetworks(mojom::CoinType::SOL),
              ElementsAreArray<std::string>({mojom::kSolanaDevnet,
                                             mojom::kSolanaTestnet,
                                             mojom::kLocalhostChainId}));
  EXPECT_THAT(network_manager()->GetHiddenNetworks(mojom::CoinType::BTC),
              ElementsAreArray<std::string>({mojom::kBitcoinTestnet}));
  EXPECT_THAT(network_manager()->GetHiddenNetworks(mojom::CoinType::ZEC),
              ElementsAreArray<std::string>({mojom::kZCashTestnet}));
  EXPECT_TRUE(AllCoinsTested());

  for (auto coin : kAllCoins) {
    for (auto& default_hidden : network_manager()->GetHiddenNetworks(coin)) {
      network_manager()->RemoveHiddenNetwork(coin, default_hidden);
    }

    EXPECT_THAT(network_manager()->GetHiddenNetworks(coin),
                ElementsAreArray<std::string>({}));

    network_manager()->AddHiddenNetwork(coin, "0x123");
    EXPECT_THAT(network_manager()->GetHiddenNetworks(coin),
                ElementsAreArray({"0x123"}));
    network_manager()->AddHiddenNetwork(coin, "0x123");
    EXPECT_THAT(network_manager()->GetHiddenNetworks(coin),
                ElementsAreArray({"0x123"}));

    network_manager()->RemoveHiddenNetwork(coin, "0x555");
    EXPECT_THAT(network_manager()->GetHiddenNetworks(coin),
                ElementsAreArray({"0x123"}));

    network_manager()->AddHiddenNetwork(coin, "0x7");
    EXPECT_THAT(network_manager()->GetHiddenNetworks(coin),
                ElementsAreArray({"0x123", "0x7"}));

    network_manager()->RemoveHiddenNetwork(coin, "0x123");
    EXPECT_THAT(network_manager()->GetHiddenNetworks(coin),
                ElementsAreArray({"0x7"}));

    network_manager()->RemoveHiddenNetwork(coin, "0x7");
    EXPECT_THAT(network_manager()->GetHiddenNetworks(coin),
                ElementsAreArray<std::string>({}));
  }
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
