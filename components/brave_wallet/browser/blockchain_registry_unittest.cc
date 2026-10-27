/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/blockchain_registry.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_wallet/browser/blockchain_list_parser.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using testing::ElementsAreArray;

namespace brave_wallet {

namespace {

const char kTokenListJson[] = R"(
  {
    "0x1": {
      "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d": {
        "name": "Crypto Kitties",
        "logo": "CryptoKitties-Kitty-13733.svg",
        "erc20": false,
        "erc721": true,
        "symbol": "CK",
        "decimals": 0
      },
     "0x0D8775F648430679A709E98d2b0Cb6250d2887EF": {
        "name": "Basic Attention Token",
        "logo": "bat.svg",
        "erc20": true,
        "symbol": "BAT",
        "decimals": 18
      },
      "0x6090A6e47849629b7245Dfa1Ca21D94cd15878Ef": {
        "name": "ENS Registrar",
        "logo": "ens.svg"
      }
    },
    "0xaa36a7": {
      "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984": {
        "name": "Uniswap",
        "logo": "uni.svg",
        "erc20": true,
        "symbol": "UNI",
        "decimals": 18
      }
    },
    "0x89": {
      "0xc2132D05D31c914a87C6611C10748AEb04B58e8F": {
          "name": "Tether USD - PoS",
          "logo": "usdt.png",
          "erc20": true,
          "symbol": "USDT",
          "decimals": 6,
          "coingeckoId": "tether"
      }
    },
    "0x65": {
      "So11111111111111111111111111111111111111112": {
        "name": "Wrapped SOL",
        "logo": "So11111111111111111111111111111111111111112.png",
        "erc20": false,
        "symbol": "SOL",
        "decimals": 9,
        "coingeckoId": "solana"
      },
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v": {
        "name": "USD Coin",
        "logo": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v.png",
        "erc20": false,
        "symbol": "USDC",
        "decimals": 6,
        "coingeckoId": "usd-coin"
      },
      "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ": {
        "name": "Tesla Inc.",
        "logo": "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ.png",
        "erc20": false,
        "symbol": "TSLA",
        "decimals": 8
      },
      "2kMpEJCZL8vEDZe7YPLMCS9Y3WKSAMedXBn7xHPvsWvi": {
        "name": "SolarMoon",
        "logo": "2kMpEJCZL8vEDZe7YPLMCS9Y3WKSAMedXBn7xHPvsWvi.png",
        "erc20": false,
        "symbol": "MOON",
        "decimals": 5,
        "token2022": true
      }
    }
  })";

mojom::BlockchainTokenPtr WrappedSolToken() {
  return mojom::BlockchainToken::New(
      "So11111111111111111111111111111111111111112", "Wrapped SOL",
      "So11111111111111111111111111111111111111112.png", false, false, false,
      false, mojom::SPLTokenProgram::kToken, false, false, "SOL", 9, true, "",
      "solana", "0x65", mojom::CoinType::SOL, mojom::ZCashTokenType::kNone);
}

mojom::BlockchainTokenPtr UsdcToken() {
  return mojom::BlockchainToken::New(
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v", "USD Coin",
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v.png", false, false, false,
      false, mojom::SPLTokenProgram::kToken, false, false, "USDC", 6, true, "",
      "usd-coin", "0x65", mojom::CoinType::SOL, mojom::ZCashTokenType::kNone);
}

mojom::BlockchainTokenPtr TslaToken() {
  return mojom::BlockchainToken::New(
      "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ", "Tesla Inc.",
      "2inRoG4DuMRRzZxAt913CCdNZCu2eGsDD9kZTrsj2DAZ.png", false, false, false,
      false, mojom::SPLTokenProgram::kToken, false, false, "TSLA", 8, true, "",
      "", "0x65", mojom::CoinType::SOL, mojom::ZCashTokenType::kNone);
}

mojom::BlockchainTokenPtr MoonToken() {
  return mojom::BlockchainToken::New(
      "2kMpEJCZL8vEDZe7YPLMCS9Y3WKSAMedXBn7xHPvsWvi", "SolarMoon",
      "2kMpEJCZL8vEDZe7YPLMCS9Y3WKSAMedXBn7xHPvsWvi.png", false, false, false,
      false, mojom::SPLTokenProgram::kToken2022, false, false, "MOON", 5, true,
      "", "", "0x65", mojom::CoinType::SOL, mojom::ZCashTokenType::kNone);
}

const char kChainListJson[] = R"(
  [
    {
      "name": "Ethereum Mainnet",
      "chain": "ETH",
      "icon": "ethereum",
      "rpc": [
        "https://mainnet.infura.io/v3/${INFURA_API_KEY}",
        "wss://mainnet.infura.io/ws/v3/${INFURA_API_KEY}",
        "https://api.mycryptoapi.com/eth",
        "https://cloudflare-eth.com"
      ],
      "faucets": [],
      "nativeCurrency": { "name": "Ether", "symbol": "ETH", "decimals": 18 },
      "infoURL": "https://ethereum.org",
      "shortName": "eth",
      "chainId": 1,
      "networkId": 1,
      "slip44": 60,
      "ens": { "registry": "0x00000000000C2E074eC69A0dFb2997BA6C7d2e1e" },
      "explorers": [
        {
          "name": "etherscan",
          "url": "https://etherscan.io",
          "standard": "EIP3091"
        }
      ]
    },
    {
      "name": "Polygon Mainnet",
      "chain": "Polygon",
      "rpc": [
        "https://polygon-rpc.com/",
        "https://rpc-mainnet.matic.network",
        "https://matic-mainnet.chainstacklabs.com",
        "https://rpc-mainnet.maticvigil.com",
        "https://rpc-mainnet.matic.quiknode.pro",
        "https://matic-mainnet-full-rpc.bwarelabs.com"
      ],
      "faucets": [],
      "nativeCurrency": { "name": "MATIC", "symbol": "MATIC", "decimals": 18 },
      "infoURL": "https://polygon.technology/",
      "shortName": "MATIC",
      "chainId": 137,
      "networkId": 137,
      "slip44": 966,
      "explorers": [
        {
          "name": "polygonscan",
          "url": "https://polygonscan.com",
          "standard": "EIP3091"
        }
      ]
    }
  ])";

const char kCoingeckoIdsMapJson[] = R"({
  "0xa": {
    "0x7f5c764cbc14f9669b88837ca1490cca17c31607": "usd-coin"
  },
  "0x65": {
    "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v": "usd-coin"
  }
})";

std::vector<std::string> GetChainIds(
    const std::vector<mojom::NetworkInfoPtr>& networks) {
  std::vector<std::string> result;
  for (auto& network : networks) {
    result.push_back(network->chain_id);
  }
  return result;
}

}  // namespace

TEST(BlockchainRegistryUnitTest, GetAllTokens) {
  base::test::TaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  ASSERT_TRUE(ParseTokenList(kTokenListJson, &token_list_map));
  registry->UpdateTokenList(std::move(token_list_map));

  // Loop twice to make sure getting the same list twice works
  // For example, make sure nothing is std::move'd
  for (size_t i = 0; i < 2; ++i) {
    base::RunLoop run_loop;
    registry->GetAllTokens(
        mojom::kMainnetChainId, mojom::CoinType::ETH,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::BlockchainTokenPtr> token_list) {
              // ENS Registrar should not be parsed because it doesn't
              // have decimals nor a symbol defined
              ASSERT_EQ(token_list.size(), 2UL);

              EXPECT_EQ(token_list[0]->name, "Crypto Kitties");
              EXPECT_EQ(token_list[0]->contract_address,
                        "0x06012c8cf97BEaD5deAe237070F9587f8E7A266d");
              EXPECT_FALSE(token_list[0]->is_erc20);
              EXPECT_TRUE(token_list[0]->is_erc721);
              EXPECT_EQ(token_list[0]->symbol, "CK");
              EXPECT_EQ(token_list[0]->decimals, 0);
              EXPECT_EQ(token_list[0]->spl_token_program,
                        mojom::SPLTokenProgram::kUnsupported);

              EXPECT_EQ(token_list[1]->name, "Basic Attention Token");
              EXPECT_EQ(token_list[1]->contract_address,
                        "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
              EXPECT_TRUE(token_list[1]->is_erc20);
              EXPECT_FALSE(token_list[1]->is_erc721);
              EXPECT_EQ(token_list[1]->symbol, "BAT");
              EXPECT_EQ(token_list[1]->decimals, 18);
              EXPECT_EQ(token_list[1]->spl_token_program,
                        mojom::SPLTokenProgram::kUnsupported);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  // Can get other chain tokens
  base::RunLoop run_loop2;
  registry->GetAllTokens(
      mojom::kSepoliaChainId, mojom::CoinType::ETH,
      base::BindLambdaForTesting(
          [&](std::vector<mojom::BlockchainTokenPtr> token_list) {
            ASSERT_EQ(token_list.size(), 1UL);
            EXPECT_EQ(token_list[0]->name, "Uniswap");
            EXPECT_EQ(token_list[0]->contract_address,
                      "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984");
            EXPECT_TRUE(token_list[0]->is_erc20);
            EXPECT_FALSE(token_list[0]->is_erc721);
            EXPECT_EQ(token_list[0]->symbol, "UNI");
            EXPECT_EQ(token_list[0]->decimals, 18);
            EXPECT_EQ(token_list[0]->spl_token_program,
                      mojom::SPLTokenProgram::kUnsupported);
            run_loop2.Quit();
          }));
  run_loop2.Run();

  // chainId which has no tokens
  base::RunLoop run_loop3;
  registry->GetAllTokens(
      "0x5", mojom::CoinType::ETH,
      base::BindLambdaForTesting(
          [&](std::vector<mojom::BlockchainTokenPtr> token_list) {
            // ENS Registrar should not be parsed because it doesn't have
            // decimals nor a symbol defined
            EXPECT_EQ(token_list.size(), 0UL);
            run_loop3.Quit();
          }));
  run_loop3.Run();

  // Get Solana tokens
  base::RunLoop run_loop4;
  registry->GetAllTokens(
      mojom::kSolanaMainnet, mojom::CoinType::SOL,
      base::BindLambdaForTesting(
          [&](std::vector<mojom::BlockchainTokenPtr> token_list) {
            ASSERT_EQ(token_list.size(), 4UL);
            EXPECT_EQ(token_list[0], TslaToken());
            EXPECT_EQ(token_list[1], MoonToken());
            EXPECT_EQ(token_list[2], UsdcToken());
            EXPECT_EQ(token_list[3], WrappedSolToken());
            run_loop4.Quit();
          }));
  run_loop4.Run();
}

TEST(BlockchainRegistryUnitTest, GetTokenByAddress) {
  base::test::TaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  ASSERT_TRUE(ParseTokenList(kTokenListJson, &token_list_map));
  registry->UpdateTokenList(std::move(token_list_map));
  base::RunLoop run_loop;
  registry->GetTokenByAddress(
      mojom::kMainnetChainId, mojom::CoinType::ETH,
      "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token->symbol, "BAT");
        run_loop.Quit();
      }));
  run_loop.Run();

  // Can get other chain tokens
  base::RunLoop run_loop2;
  registry->GetTokenByAddress(
      mojom::kSepoliaChainId, mojom::CoinType::ETH,
      "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token->symbol, "UNI");
        run_loop2.Quit();
      }));
  run_loop2.Run();

  // tokens for chanId exist but address doesn't exist
  base::RunLoop run_loop3;
  registry->GetTokenByAddress(
      mojom::kMainnetChainId, mojom::CoinType::ETH,
      "0xCCC775F648430679A709E98d2b0Cb6250d2887EF",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_FALSE(token);
        run_loop3.Quit();
      }));
  run_loop3.Run();

  // chainId which has no tokens
  base::RunLoop run_loop4;
  registry->GetTokenByAddress(
      mojom::kSepoliaChainId, mojom::CoinType::ETH,
      "0xCCC775F648430679A709E98d2b0Cb6250d2887EF",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_FALSE(token);
        run_loop4.Quit();
      }));
  run_loop4.Run();

  // Get Solana token
  base::RunLoop run_loop5;
  registry->GetTokenByAddress(
      mojom::kSolanaMainnet, mojom::CoinType::SOL,
      "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token, UsdcToken());
        run_loop5.Quit();
      }));
  run_loop5.Run();
}

TEST(BlockchainRegistryUnitTest, GetTokenBySymbol) {
  base::test::TaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  ASSERT_TRUE(ParseTokenList(kTokenListJson, &token_list_map));
  registry->UpdateTokenList(std::move(token_list_map));
  base::RunLoop run_loop;
  registry->GetTokenBySymbol(
      mojom::kMainnetChainId, mojom::CoinType::ETH, "BAT",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token->contract_address,
                  "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
        run_loop.Quit();
      }));
  run_loop.Run();

  // Can get other chain tokens
  base::RunLoop run_loop2;
  registry->GetTokenBySymbol(
      mojom::kSepoliaChainId, mojom::CoinType::ETH, "UNI",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token->contract_address,
                  "0x1f9840a85d5aF5bf1D1762F925BDADdC4201F984");
        run_loop2.Quit();
      }));
  run_loop2.Run();

  // chainId has tokens but token doesn't exist
  base::RunLoop run_loop3;
  registry->GetTokenBySymbol(
      mojom::kMainnetChainId, mojom::CoinType::ETH, "BRB",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_FALSE(token);
        run_loop3.Quit();
      }));
  run_loop3.Run();

  // chainId which has no tokens
  base::RunLoop run_loop4;
  registry->GetTokenBySymbol(
      "0x5", mojom::CoinType::ETH, "BRB",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_FALSE(token);
        run_loop4.Quit();
      }));
  run_loop4.Run();

  // Get Solana token
  base::RunLoop run_loop5;
  registry->GetTokenBySymbol(
      mojom::kSolanaMainnet, mojom::CoinType::SOL, "USDC",
      base::BindLambdaForTesting([&](mojom::BlockchainTokenPtr token) {
        EXPECT_EQ(token, UsdcToken());
        run_loop5.Quit();
      }));
  run_loop5.Run();
}

TEST(BlockchainRegistryUnitTest, IsRestrictedAddress) {
  base::test::TaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();

  // Before parsing the list, should return false.
  EXPECT_FALSE(registry->IsRestrictedAddress(
      "0xb9ef770b6a5e12e45983c5d80545258aa38f3b78"));

  // After parsing the list, we should have some addresses;
  std::vector<std::string> input_list;
  input_list.push_back("0xb9ef770b6a5e12e45983c5d80545258aa38f3b78");
  BlockchainRegistry::ScopedRestrictedAddressesForTesting scoped_restricted(
      input_list);
  EXPECT_TRUE(registry->IsRestrictedAddress(
      "0xb9ef770b6a5e12e45983c5d80545258aa38f3b78"));
  EXPECT_TRUE(registry->IsRestrictedAddress(
      "0xb9ef770b6a5e12e45983c5d80545258aa38f3b78"));
  EXPECT_FALSE(registry->IsRestrictedAddress(""));
}

TEST(BlockchainRegistryUnitTest, GetPrepopulatedNetworks) {
  base::test::TaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();

  ChainList chain_list;
  ASSERT_TRUE(ParseChainList(kChainListJson, &chain_list));
  registry->UpdateChainList(std::move(chain_list));

  EXPECT_THAT(GetChainIds(registry->GetPrepopulatedNetworks()),
              ElementsAreArray({"0x1", "0x89"}));
}

TEST(BlockchainRegistryUnitTest, GetPrepopulatedNetworksKnownOnesArePreferred) {
  base::test::TaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();

  ChainList chain_list;
  ASSERT_TRUE(ParseChainList(kChainListJson, &chain_list));
  chain_list[0]->chain_name = "Custom Name";
  registry->UpdateChainList(std::move(chain_list));

  auto found_networks = registry->GetPrepopulatedNetworks();
  EXPECT_EQ(2u, found_networks.size());
  EXPECT_EQ(found_networks[0]->chain_name, "Ethereum Mainnet");
}

TEST(BlockchainRegistryUnitTest, GetEthTokenListMap) {
  base::test::TaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();
  TokenListMap token_list_map;
  ASSERT_TRUE(ParseTokenList(kTokenListJson, &token_list_map));
  registry->UpdateTokenList(std::move(token_list_map));

  // Loop twice to make sure getting the same list twice works
  // For example, make sure nothing is std::move'd
  for (size_t i = 0; i < 2; i++) {
    token_list_map = registry->GetEthTokenListMap({mojom::kMainnetChainId});
    EXPECT_EQ(token_list_map.size(), 1UL);
    EXPECT_EQ(token_list_map[mojom::kMainnetChainId].size(), 2UL);
  }
}

TEST(BlockchainRegistryUnitTest, GetCoingeckoId) {
  base::test::TaskEnvironment task_environment;
  auto* registry = BlockchainRegistry::GetInstance();
  std::optional<CoingeckoIdsMap> coingecko_ids_map =
      ParseCoingeckoIdsMap(kCoingeckoIdsMapJson);
  ASSERT_TRUE(coingecko_ids_map);
  registry->UpdateCoingeckoIdsMap(std::move(*coingecko_ids_map));

  // Chain: ✅
  // Contract: ✅
  // Result: ✅
  EXPECT_EQ(
      registry->GetCoingeckoId(mojom::kOptimismMainnetChainId,
                               "0x7f5c764cbc14f9669b88837ca1490cca17c31607"),
      "usd-coin");

  // Chain: ✅
  // Contract: ❌
  // Result: ❌
  EXPECT_EQ(
      registry->GetCoingeckoId(mojom::kOptimismMainnetChainId, "0xdeadbeef"),
      std::nullopt);

  // Chain: ❌
  // Contract: ✅
  // Result: ❌
  EXPECT_EQ(registry->GetCoingeckoId(
                "0xdeadbeef", "0x7f5c764cbc14f9669b88837ca1490cca17c31607"),
            std::nullopt);

  // Chain: ❌
  // Contract: ❌
  // Result: ❌
  EXPECT_EQ(registry->GetCoingeckoId("0xdeadbeef", "0xcafebabe"), std::nullopt);

  // Chain: ✅ (wrong case)
  // Contract: ✅
  // Result: ✅
  EXPECT_EQ(registry->GetCoingeckoId(
                "0xA", "0x7f5c764cbc14f9669b88837ca1490cca17c31607"),
            "usd-coin");

  // Chain: ✅
  // Contract: ✅ (mixed case EIP-55)
  // Result: ✅
  EXPECT_EQ(
      registry->GetCoingeckoId(mojom::kOptimismMainnetChainId,
                               "0x7F5c764cBc14f9669B88837ca1490cCa17c31607"),
      "usd-coin");

  // Chain: ✅
  // Contract: ✅ (mixed case Solana)
  // Result: ✅
  EXPECT_EQ(
      registry->GetCoingeckoId(mojom::kSolanaMainnet,
                               "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v"),
      "usd-coin");

  // Chain: ✅
  // Contract: ✅ (wrong case Solana)
  // Result: ✅
  EXPECT_EQ(
      registry->GetCoingeckoId(mojom::kSolanaMainnet,
                               "epjfwdd5aufqssqem2qn1xzybapc8g4weggkzwytdt1v"),
      "usd-coin");
}

TEST(BlockchainRegistryUnitTest, ParseLists) {
  base::test::TaskEnvironment task_environment;
  base::ScopedTempDir install_dir;
  ASSERT_TRUE(install_dir.CreateUniqueTempDir());
  const base::FilePath path = install_dir.GetPath();

  ASSERT_TRUE(base::WriteFile(path.AppendASCII("coingecko-ids.json"),
                              kCoingeckoIdsMapJson));
  ASSERT_TRUE(
      base::WriteFile(path.AppendASCII("coingecko.json"), kTokenListJson));
  ASSERT_TRUE(
      base::WriteFile(path.AppendASCII("chainlist.json"), kChainListJson));
  ASSERT_TRUE(base::WriteFile(
      path.AppendASCII(BlockchainRegistry::kRestrictedAddressFileName),
      R"({"addresses": ["0xb9ef770b6a5e12e45983c5d80545258aa38f3b78"]})"));
  ASSERT_TRUE(base::WriteFile(
      path.AppendASCII(BlockchainRegistry::kPolkadotRpcUrlsFileName),
      R"({"polkadot_acala": ["https://acala-rpc-0.aca-api.network"]})"));

  auto* registry = BlockchainRegistry::GetInstance();
  auto run_loop = std::make_unique<base::RunLoop>();
  registry->ParseLists(path,
                       base::BindLambdaForTesting([&]() { run_loop->Quit(); }));
  run_loop->Run();

  // coingecko-ids.json
  EXPECT_EQ(
      registry->GetCoingeckoId(mojom::kOptimismMainnetChainId,
                               "0x7f5c764cbc14f9669b88837ca1490cca17c31607"),
      "usd-coin");

  // coingecko.json
  EXPECT_EQ(
      registry
          ->GetTokenByAddress(mojom::kMainnetChainId, mojom::CoinType::ETH,
                              "0x0D8775F648430679A709E98d2b0Cb6250d2887EF")
          ->symbol,
      "BAT");

  EXPECT_EQ(registry
                ->GetTokenByAddress(
                    mojom::kPolygonMainnetChainId, mojom::CoinType::ETH,
                    "0xc2132D05D31c914a87C6611C10748AEb04B58e8F")
                ->symbol,
            "USDT");

  EXPECT_EQ(
      registry
          ->GetTokenByAddress(mojom::kSolanaMainnet, mojom::CoinType::SOL,
                              "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v")
          ->symbol,
      "USDC");

  // chainlist.json
  EXPECT_THAT(GetChainIds(registry->GetPrepopulatedNetworks()),
              ElementsAreArray({"0x1", "0x89"}));

  // kRestrictedAddressFileName
  EXPECT_TRUE(registry->IsRestrictedAddress(
      "0xb9ef770b6a5e12e45983c5d80545258aa38f3b78"));

  // kPolkadotRpcUrlsFileName
  EXPECT_THAT(
      registry->GetPolkadotRpcUrls(),
      testing::ElementsAre(testing::Pair(
          mojom::kPolkadotAcala,
          ElementsAreArray({GURL("https://acala-rpc-0.aca-api.network")}))));
}

}  // namespace brave_wallet
