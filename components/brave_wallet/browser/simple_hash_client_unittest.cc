/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/simple_hash_client.h"

#include <optional>

#include "base/json/json_reader.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/features.h"
#include "content/public/test/browser_task_environment.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

class SimpleHashClientUnitTest : public testing::Test {
 public:
  SimpleHashClientUnitTest()
      : shared_url_loader_factory_(
            base::MakeRefCounted<network::WeakWrapperSharedURLLoaderFactory>(
                &url_loader_factory_)),
        task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}
  ~SimpleHashClientUnitTest() override = default;

 protected:
  void SetUp() override {
    simple_hash_client_ =
        std::make_unique<SimpleHashClient>(shared_url_loader_factory_);
  }

  void SetInterceptor(const GURL& intended_url, const std::string& content) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, intended_url, content](const network::ResourceRequest& request) {
          if (request.url.spec() == intended_url) {
            url_loader_factory_.ClearResponses();
            url_loader_factory_.AddResponse(request.url.spec(), content);
          }
        }));
  }

  void SetInterceptors(std::map<GURL, std::string> responses) {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&, responses](const network::ResourceRequest& request) {
          // If the request url is in responses, add that response
          auto it = responses.find(request.url);
          if (it != responses.end()) {
            // Get the response string
            std::string response = it->second;
            url_loader_factory_.ClearResponses();
            url_loader_factory_.AddResponse(request.url.spec(), response);
          }
        }));
  }

  void SetHTTPRequestTimeoutInterceptor() {
    url_loader_factory_.SetInterceptor(base::BindLambdaForTesting(
        [&](const network::ResourceRequest& request) {
          url_loader_factory_.ClearResponses();
          url_loader_factory_.AddResponse(request.url.spec(), "",
                                          net::HTTP_REQUEST_TIMEOUT);
        }));
  }

  void TestFetchAllNFTsFromSimpleHash(
      const std::string& account_address,
      const std::vector<std::string>& chain_ids,
      mojom::CoinType coin,
      const std::vector<mojom::BlockchainTokenPtr>& expected_nfts) {
    base::RunLoop run_loop;
    simple_hash_client_->FetchAllNFTsFromSimpleHash(
        account_address, chain_ids, coin,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::BlockchainTokenPtr> nfts) {
              ASSERT_EQ(nfts.size(), expected_nfts.size());
              EXPECT_EQ(nfts, expected_nfts);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestFetchNFTsFromSimpleHash(
      const std::string& account_address,
      const std::vector<std::string>& chain_ids,
      mojom::CoinType coin,
      std::optional<std::string> cursor,
      bool skip_spam,
      bool only_spam,
      const std::vector<mojom::BlockchainTokenPtr>& expected_nfts,
      std::optional<std::string> expected_cursor) {
    base::RunLoop run_loop;
    simple_hash_client_->FetchNFTsFromSimpleHash(
        account_address, chain_ids, coin, cursor, skip_spam, only_spam,
        base::BindLambdaForTesting(
            [&](std::vector<mojom::BlockchainTokenPtr> nfts,
                const std::optional<std::string>& returned_cursor) {
              ASSERT_EQ(nfts.size(), expected_nfts.size());
              EXPECT_EQ(returned_cursor, expected_cursor);
              EXPECT_EQ(nfts, expected_nfts);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<SimpleHashClient> simple_hash_client_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(SimpleHashClientUnitTest, GetSimpleHashNftsByWalletUrl) {
  // Empty address yields empty URL
  EXPECT_EQ(simple_hash_client_->GetSimpleHashNftsByWalletUrl(
                "", {mojom::kMainnetChainId}, std::nullopt),
            GURL(""));

  // Empty chains yields empty URL
  EXPECT_EQ(simple_hash_client_->GetSimpleHashNftsByWalletUrl(
                "0x0000000000000000000000000000000000000000", {}, std::nullopt),
            GURL());

  // One valid chain yields correct URL
  EXPECT_EQ(simple_hash_client_->GetSimpleHashNftsByWalletUrl(
                "0x0000000000000000000000000000000000000000",
                {mojom::kMainnetChainId}, std::nullopt),
            GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
                 "owners?chains=ethereum&wallet_addresses="
                 "0x0000000000000000000000000000000000000000"));

  // Two valid chains yields correct URL
  EXPECT_EQ(simple_hash_client_->GetSimpleHashNftsByWalletUrl(
                "0x0000000000000000000000000000000000000000",
                {mojom::kMainnetChainId, mojom::kOptimismMainnetChainId},
                std::nullopt),
            GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
                 "owners?chains=ethereum%2Coptimism&wallet_addresses="
                 "0x0000000000000000000000000000000000000000"));

  // One invalid chain yields empty URL
  EXPECT_EQ(simple_hash_client_->GetSimpleHashNftsByWalletUrl(
                "0x0000000000000000000000000000000000000000",
                {"chain ID not supported by SimpleHash"}, std::nullopt),
            GURL());

  // One valid chain with cursor yields correct URL
  std::optional<std::string> cursor = "example_cursor";
  EXPECT_EQ(
      simple_hash_client_->GetSimpleHashNftsByWalletUrl(
          "0x0000000000000000000000000000000000000000",
          {mojom::kMainnetChainId}, cursor),
      GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
           "owners?chains=ethereum&wallet_addresses="
           "0x0000000000000000000000000000000000000000&cursor=example_cursor"));

  // Two valid chains with cursor yields correct URL
  EXPECT_EQ(
      simple_hash_client_->GetSimpleHashNftsByWalletUrl(
          "0x0000000000000000000000000000000000000000",
          {mojom::kMainnetChainId, mojom::kOptimismMainnetChainId}, cursor),
      GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
           "owners?chains=ethereum%2Coptimism&wallet_addresses="
           "0x0000000000000000000000000000000000000000&cursor=example_cursor"));
}

TEST_F(SimpleHashClientUnitTest, ParseNFTsFromSimpleHash) {
  std::string json;
  std::optional<base::Value> json_value;

  // Non dictionary JSON response yields nullopt
  json = R"([])";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  auto result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH, true, false);
  ASSERT_FALSE(result);

  // Missing 'nfts' key yields nullopt
  json = R"({"foo": "bar"})";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH, true, false);
  ASSERT_FALSE(result);

  // Dictionary type 'nfts' key yields nullopt
  json = R"({"nfts": {}})";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH, true, false);
  ASSERT_FALSE(result);

  // Missing next_cursor yields empty next_cursor
  json = R"({
    "next": "https://foo.com/api/v0/nfts/owners?chains=ethereum&wallet_addresses=0x00",
    "previous": null,
    "nfts": [
      {
        "chain": "polygon",
        "contract_address": "0x1111111111111111111111111111111111111111",
        "token_id": "1",
        "name": "Token #1",
        "image_url": "https://nftimages-cdn.simplehash.com/1.png",
        "contract": {
          "type": "ERC721",
          "symbol": "ONE"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH, true, false);
  ASSERT_TRUE(result);
  ASSERT_FALSE(result->first);

  // Null next cursor yields empty next cursor
  json = R"({
    "next": "http://api.simplehash.com/api/v0/nfts/owners?chains=ethereum&wallet_addresses=0x00",
    "next_cursor": null,
    "previous": null,
    "nfts": [
      {
        "chain": "polygon",
        "contract_address": "0x1111111111111111111111111111111111111111",
        "token_id": "1",
        "name": "Token #1",
        "image_url": "https://nftimages-cdn.simplehash.com/1.png",
        "contract": {
          "type": "ERC721",
          "symbol": "ONE"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH, true, false);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->first, std::nullopt);

  // Unsupported CoinType yields nullopt (valid otherwise)
  json = R"({
    "next": null,
    "next_cursor": "abc123",
    "previous": null,
    "nfts": [
      {
        "chain": "polygon",
        "contract_address": "0x1111111111111111111111111111111111111111",
        "token_id": "1",
        "name": "Token #1",
        "image_url": "https://nftimages-cdn.simplehash.com/1.png",
        "contract": {
          "type": "ERC721",
          "symbol": "ONE"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::FIL, true, false);

  // Valid, 1 ETH NFT
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH, true, false);
  ASSERT_TRUE(result);
  ASSERT_TRUE(result->first);
  EXPECT_EQ(result->first, "abc123");
  EXPECT_EQ(result->second.size(), 1u);
  EXPECT_EQ(result->second[0]->contract_address,
            "0x1111111111111111111111111111111111111111");
  EXPECT_EQ(result->second[0]->name, "Token #1");
  EXPECT_EQ(result->second[0]->logo,
            "https://nftimages-cdn.simplehash.com/1.png");
  EXPECT_EQ(result->second[0]->is_erc20, false);
  EXPECT_EQ(result->second[0]->is_erc721, true);
  EXPECT_EQ(result->second[0]->is_erc1155, false);
  EXPECT_EQ(result->second[0]->is_nft, true);
  EXPECT_EQ(result->second[0]->symbol, "ONE");
  EXPECT_EQ(result->second[0]->decimals, 0);
  EXPECT_EQ(result->second[0]->visible, true);
  EXPECT_EQ(result->second[0]->token_id, "0x1");
  EXPECT_EQ(result->second[0]->chain_id, mojom::kPolygonMainnetChainId);
  EXPECT_EQ(result->second[0]->coin, mojom::CoinType::ETH);

  // Valid, 2 ETH NFTs
  json = R"({
    "next": "https://api.simplehash.com/api/v0/nfts/next/abc123",
    "next_cursor": "abc123",
    "previous": null,
    "nfts": [
      {
        "chain": "polygon",
        "contract_address": "0x1111111111111111111111111111111111111111",
        "token_id": "1",
        "name": "Token #1",
        "image_url": "https://nftimages-cdn.simplehash.com/1.png",
        "contract": {
          "type": "ERC721",
          "symbol": "ONE"
        },
        "collection": {
          "spam_score": 0
        }
      },
      {
        "chain": "ethereum",
        "contract_address": "0x2222222222222222222222222222222222222222",
        "token_id": "2",
        "name": "Token #2",
        "image_url": "https://nftimages-cdn.simplehash.com/2.png",
        "contract": {
          "type": "ERC721"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH, true, false);
  ASSERT_TRUE(result);
  ASSERT_TRUE(result->first);
  EXPECT_EQ(result->first, "abc123");
  EXPECT_EQ(result->second.size(), 2u);
  EXPECT_EQ(result->second[0]->contract_address,
            "0x1111111111111111111111111111111111111111");
  EXPECT_EQ(result->second[0]->name, "Token #1");
  EXPECT_EQ(result->second[0]->logo,
            "https://nftimages-cdn.simplehash.com/1.png");
  EXPECT_EQ(result->second[0]->is_erc20, false);
  EXPECT_EQ(result->second[0]->is_erc721, true);
  EXPECT_EQ(result->second[0]->is_erc1155, false);
  EXPECT_EQ(result->second[0]->is_nft, true);
  EXPECT_EQ(result->second[0]->symbol, "ONE");
  EXPECT_EQ(result->second[0]->decimals, 0);
  EXPECT_EQ(result->second[0]->visible, true);
  EXPECT_EQ(result->second[0]->token_id, "0x1");
  EXPECT_EQ(result->second[0]->chain_id, mojom::kPolygonMainnetChainId);
  EXPECT_EQ(result->second[0]->coin, mojom::CoinType::ETH);

  EXPECT_EQ(result->second[1]->contract_address,
            "0x2222222222222222222222222222222222222222");
  EXPECT_EQ(result->second[1]->name, "Token #2");
  EXPECT_EQ(result->second[1]->logo,
            "https://nftimages-cdn.simplehash.com/2.png");
  EXPECT_EQ(result->second[1]->is_erc20, false);
  EXPECT_EQ(result->second[1]->is_erc721, true);
  EXPECT_EQ(result->second[1]->is_erc1155, false);
  EXPECT_EQ(result->second[1]->is_nft, true);
  // If symbol is null, it should be saved as an empty string
  EXPECT_EQ(result->second[1]->symbol, "");
  EXPECT_EQ(result->second[1]->decimals, 0);
  EXPECT_EQ(result->second[1]->visible, true);
  EXPECT_EQ(result->second[1]->token_id, "0x2");
  EXPECT_EQ(result->second[1]->chain_id, mojom::kMainnetChainId);
  EXPECT_EQ(result->second[1]->coin, mojom::CoinType::ETH);

  // 6 ETH nfts, but only 1 has all necessary keys yields 1 NFT
  //
  // 1. Missing nothing (valid)
  // 2. Missing chain_id
  // 3. Missing contract_address
  // 4. Missing token_id
  // 5. Missing standard
  // 6. Missing spam_score
  json = R"({
    "next": "https://api.simplehash.com/api/v0/nfts/next",
    "previous": null,
    "nfts": [
      {
        "chain": "polygon",
        "contract_address": "0x1111111111111111111111111111111111111111",
        "token_id": "1",
        "contract": {
          "type": "ERC721",
          "symbol": "ONE"
        },
        "collection": {
          "spam_score": 0
        }
      },
      {
        "contract_address": "0x2222222222222222222222222222222222222222",
        "token_id": "2",
        "contract": {
          "type": "ERC721",
          "symbol": "TWO"
        },
        "collection": {
          "spam_score": 0
        }
      },
      {
        "chain": "ethereum",
        "token_id": "3",
        "contract": {
          "type": "ERC721",
          "symbol": "THREE"
        },
        "collection": {
          "spam_score": 0
        }
      },
      {
        "chain": "ethereum",
        "contract_address": "0x4444444444444444444444444444444444444444",
        "contract": {
          "type": "ERC721",
          "symbol": "FOUR"
        },
        "collection": {
          "spam_score": 0
        }
      },
      {
        "chain": "ethereum",
        "contract_address": "0x5555555555555555555555555555555555555555",
        "token_id": "5",
        "contract": {
          "symbol": "FIVE"
        },
        "collection": {
          "spam_score": 0
        }
      },
      {
        "chain": "polygon",
        "contract_address": "0x6666666666666666666666666666666666666666",
        "token_id": "6",
        "contract": {
          "type": "ERC721",
          "symbol": "SIX"
        },
        "collection": {
        }
      }
    ]
  })";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::ETH, true, false);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->second.size(), 1u);

  // 1 SOL NFT (NonFungible)
  json = R"({
    "next": null,
    "previous": null,
    "nfts": [
      {
        "chain": "solana",
        "contract_address": "AvdAUsR4qgsT5HgyKCVeGjimmyu8xrG3RudFqm5txDDE",
        "token_id": null,
        "name": "y00t #2623",
        "description": "y00ts is a generative art project of 15,000 NFTs.",
        "image_url": "https://cdn.simplehash.com/assets/dc78fa011ba46fa12.png",
        "status": "minted",
        "contract": {
          "type": "NonFungible",
          "name": "y00t #2623",
          "symbol": "Y00T"
        },
        "collection": {
          "spam_score": 0
        },
        "extra_metadata": {
          "is_mutable": true
        }
      }
    ]
  })";

  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::SOL, true, false);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->second.size(), 1u);
  EXPECT_EQ(result->second[0]->contract_address,
            "AvdAUsR4qgsT5HgyKCVeGjimmyu8xrG3RudFqm5txDDE");
  EXPECT_EQ(result->second[0]->name, "y00t #2623");
  EXPECT_EQ(result->second[0]->logo,
            "https://cdn.simplehash.com/assets/"
            "dc78fa011ba46fa12.png");
  EXPECT_EQ(result->second[0]->is_erc20, false);
  EXPECT_EQ(result->second[0]->is_erc721, false);
  EXPECT_EQ(result->second[0]->is_erc1155, false);
  EXPECT_EQ(result->second[0]->is_nft, true);
  EXPECT_EQ(result->second[0]->symbol, "Y00T");
  EXPECT_EQ(result->second[0]->decimals, 0);
  EXPECT_EQ(result->second[0]->visible, true);
  EXPECT_EQ(result->second[0]->token_id, "");
  EXPECT_EQ(result->second[0]->coingecko_id, "");
  EXPECT_EQ(result->second[0]->chain_id, mojom::kSolanaMainnet);
  EXPECT_EQ(result->second[0]->coin, mojom::CoinType::SOL);

  // 1 SOL NFT (NonFungibleEdition)
  json = R"({
    "next": null,
    "previous": null,
    "nfts": [
      {
        "nft_id": "solana.g9qugQPwCsw6JEUEXSJ2ngQ7TTqzdv69pDGfDaQ2oCe",
        "chain": "solana",
        "contract_address": "g9qugQPwCsw6JEUEXSJ2ngQ7TTqzdv69pDGfDaQ2oCe",
        "token_id": null,
        "name": "Boba Guys @ Solana Clubhouse",
        "description": "Sign-up for early access to the Boba Guys Passport",
        "image_url": "https://cdn.simplehash.com/assets/a3a7c3232c42963d747054c08dd219c795cf76c3b6fbdc77d5de9baa50e1a174.jpg",
        "contract": {
          "type": "NonFungibleEdition",
          "name": "Boba Guys @ Solana Clubhouse",
          "symbol": "BGSC"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::SOL, true, false);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->second.size(), 1u);
  EXPECT_EQ(result->second[0]->contract_address,
            "g9qugQPwCsw6JEUEXSJ2ngQ7TTqzdv69pDGfDaQ2oCe");
  EXPECT_EQ(result->second[0]->name, "Boba Guys @ Solana Clubhouse");
  EXPECT_EQ(
      result->second[0]->logo,
      "https://cdn.simplehash.com/assets/"
      "a3a7c3232c42963d747054c08dd219c795cf76c3b6fbdc77d5de9baa50e1a174.jpg");
  EXPECT_EQ(result->second[0]->is_erc20, false);
  EXPECT_EQ(result->second[0]->is_erc721, false);
  EXPECT_EQ(result->second[0]->is_erc1155, false);
  EXPECT_EQ(result->second[0]->is_nft, true);
  EXPECT_EQ(result->second[0]->symbol, "BGSC");
  EXPECT_EQ(result->second[0]->decimals, 0);
  EXPECT_EQ(result->second[0]->visible, true);
  EXPECT_EQ(result->second[0]->token_id, "");
  EXPECT_EQ(result->second[0]->coingecko_id, "");
  EXPECT_EQ(result->second[0]->chain_id, mojom::kSolanaMainnet);
  EXPECT_EQ(result->second[0]->coin, mojom::CoinType::SOL);

  // 1 SOL NFT (ProgrammableNonFungible)
  json = R"({
    "next": null,
    "previous": null,
    "nfts": [
      {
        "nft_id": "solana.BHWBJ7XtBqJJbg9SrAUH4moeF8VpJo3WXyDh6vc1qqLG",
        "chain": "solana",
        "contract_address": "BHWBJ7XtBqJJbg9SrAUH4moeF8VpJo3WXyDh6vc1qqLG",
        "token_id": null,
        "name": "Mad Lads #8752",
        "description": "Fock it.",
        "image_url": "https://cdn.simplehash.com/assets/6fa3b325fd715c0b967988ad76c668b9cf41acb7aeff646ab4135095afd1dea5.png",
        "contract": {
          "type": "ProgrammableNonFungible",
          "name": "Mad Lad #8752",
          "symbol": "MAD",
          "deployed_by": null,
          "deployed_via_contract": null
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::SOL, true, false);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->second.size(), 1u);
  EXPECT_EQ(result->second[0]->contract_address,
            "BHWBJ7XtBqJJbg9SrAUH4moeF8VpJo3WXyDh6vc1qqLG");
  EXPECT_EQ(result->second[0]->name, "Mad Lads #8752");
  EXPECT_EQ(
      result->second[0]->logo,
      "https://cdn.simplehash.com/assets/"
      "6fa3b325fd715c0b967988ad76c668b9cf41acb7aeff646ab4135095afd1dea5.png");
  EXPECT_EQ(result->second[0]->is_erc20, false);
  EXPECT_EQ(result->second[0]->is_erc721, false);
  EXPECT_EQ(result->second[0]->is_erc1155, false);
  EXPECT_EQ(result->second[0]->is_nft, true);
  EXPECT_EQ(result->second[0]->symbol, "MAD");
  EXPECT_EQ(result->second[0]->decimals, 0);
  EXPECT_EQ(result->second[0]->visible, true);
  EXPECT_EQ(result->second[0]->token_id, "");
  EXPECT_EQ(result->second[0]->coingecko_id, "");
  EXPECT_EQ(result->second[0]->chain_id, mojom::kSolanaMainnet);
  EXPECT_EQ(result->second[0]->coin, mojom::CoinType::SOL);

  // An NFT with a spam_score > 0 will be skipped
  json = R"({
    "next": null,
    "previous": null,
    "nfts": [
      {
        "chain": "solana",
        "contract_address": "AvdAUsR4qgsT5HgyKCVeGjimmyu8xrG3RudFqm5txDDE",
        "token_id": null,
        "name": "y00t #2623",
        "description": "y00ts is a generative art project of 15,000 NFTs.",
        "image_url": "https://cdn.simplehash.com/assets/dc78fa011ba46fa12.png",
        "status": "minted",
        "contract": {
          "type": "NonFungible",
          "name": "y00t #2623",
          "symbol": "Y00T"
        },
        "collection": {
          "spam_score": 100
        },
        "extra_metadata": {
          "is_mutable": true
        }
      },
      {
        "nft_id": "solana.BHWBJ7XtBqJJbg9SrAUH4moeF8VpJo3WXyDh6vc1qqLG",
        "chain": "solana",
        "contract_address": "BHWBJ7XtBqJJbg9SrAUH4moeF8VpJo3WXyDh6vc1qqLG",
        "token_id": null,
        "name": "Mad Lads #8752",
        "description": "Fock it.",
        "image_url": "https://cdn.simplehash.com/assets/6fa3b325fd715c0b967988ad76c668b9cf41acb7aeff646ab4135095afd1dea5.png",
        "contract": {
          "type": "ProgrammableNonFungible",
          "name": "Mad Lad #8752",
          "symbol": "MAD",
          "deployed_by": null,
          "deployed_via_contract": null
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  json_value = base::JSONReader::Read(json);
  ASSERT_TRUE(json_value);

  // When skip_spam is true and only_spam is false, non spam token should be
  // parsed
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::SOL, true, false);
  ASSERT_TRUE(result);
  ASSERT_EQ(result->second.size(), 1u);
  EXPECT_EQ(result->second[0]->contract_address,
            "BHWBJ7XtBqJJbg9SrAUH4moeF8VpJo3WXyDh6vc1qqLG");
  EXPECT_FALSE(result->second[0]->is_spam);

  // When skip_spam is false and only_spam is true, spam token should be parsed
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::SOL, false, true);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->second.size(), 1u);
  EXPECT_EQ(result->second[0]->contract_address,
            "AvdAUsR4qgsT5HgyKCVeGjimmyu8xrG3RudFqm5txDDE");
  EXPECT_FALSE(result->second[0]->is_spam);

  // When only_spam is set and skip_spam is set, parsing should fail
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::SOL, true, true);
  ASSERT_FALSE(result);

  // When only_spam is false and skip_spam is false, spam and non spam should be
  // parsed
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      *json_value, mojom::CoinType::SOL, false, false);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->second.size(), 2u);
  EXPECT_EQ(result->second[0]->contract_address,
            "AvdAUsR4qgsT5HgyKCVeGjimmyu8xrG3RudFqm5txDDE");
  EXPECT_FALSE(result->second[0]->is_spam);
  EXPECT_EQ(result->second[1]->contract_address,
            "BHWBJ7XtBqJJbg9SrAUH4moeF8VpJo3WXyDh6vc1qqLG");
  EXPECT_FALSE(result->second[1]->is_spam);
}

TEST_F(SimpleHashClientUnitTest, FetchAllNFTsFromSimpleHash) {
  std::vector<mojom::BlockchainTokenPtr> expected_nfts;
  std::string json;
  std::string json2;
  std::map<GURL, std::string> responses;
  GURL url;

  // Empty account address yields empty expected_nfts
  TestFetchAllNFTsFromSimpleHash("", {mojom::kMainnetChainId},
                                 mojom::CoinType::ETH, expected_nfts);

  // Empty chain IDs yields empty expected_nfts
  TestFetchAllNFTsFromSimpleHash("0x0000000000000000000000000000000000000000",
                                 {}, mojom::CoinType::ETH, expected_nfts);

  // Unsupported chain ID yields empty expected_nfts
  TestFetchAllNFTsFromSimpleHash("0x0000000000000000000000000000000000000000",
                                 {}, mojom::CoinType::FIL, expected_nfts);

  // Non 2xx response yields empty expected_nfts
  SetHTTPRequestTimeoutInterceptor();
  TestFetchAllNFTsFromSimpleHash("0x0000000000000000000000000000000000000000",
                                 {mojom::kMainnetChainId}, mojom::CoinType::ETH,
                                 expected_nfts);

  // 1 NFT is parsed
  json = R"({
    "next": null,
    "previous": null,
    "nfts": [
      {
        "chain": "polygon",
        "contract_address": "0x1111111111111111111111111111111111111111",
        "token_id": "1",
        "contract": {
          "type": "ERC721",
          "symbol": "ONE"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  auto nft1 = mojom::BlockchainToken::New();
  nft1->chain_id = mojom::kPolygonMainnetChainId;
  nft1->contract_address = "0x1111111111111111111111111111111111111111";
  nft1->token_id = "0x1";
  nft1->is_erc721 = true;
  nft1->is_erc1155 = false;
  nft1->is_erc20 = false;
  nft1->is_nft = true;
  nft1->symbol = "ONE";
  nft1->coin = mojom::CoinType::ETH;
  expected_nfts.push_back(std::move(nft1));
  url = GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "owners?chains=ethereum%2Coptimism&wallet_addresses="
      "0x0000000000000000000000000000000000000000");
  responses[url] = json;
  SetInterceptors(responses);
  TestFetchAllNFTsFromSimpleHash(
      "0x0000000000000000000000000000000000000000",
      {mojom::kMainnetChainId, mojom::kOptimismMainnetChainId},
      mojom::CoinType::ETH, expected_nfts);

  // If 'next_cursor' page url is present, it should make another request
  // Also, spam NFTs are ignored.
  responses.clear();
  json = R"({
    "next": "https://api.simplehash.com/api/v0/nfts/next",
    "next_cursor": "abc123",
    "previous": null,
    "nfts": [
      {
        "chain": "polygon",
        "contract_address": "0x1111111111111111111111111111111111111111",
        "token_id": "1",
        "contract": {
          "type": "ERC721",
          "symbol": "ONE"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  responses[url] = json;
  GURL next_url = GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "owners?chains=ethereum%2Coptimism&wallet_addresses="
      "0x0000000000000000000000000000000000000000&cursor=abc123");
  json2 = R"({
    "next": null,
    "previous": null,
    "nfts": [
      {
        "nft_id": "ethereum.0x5555555555555555555555555555555555555555.555",
        "chain": "ethereum",
        "contract_address": "0x5555555555555555555555555555555555555555",
        "token_id": "555",
        "contract": {
          "type": "ERC721",
          "symbol": "FIVE"
        },
        "collection": {
          "spam_score": 0
        }
      },
      {
        "nft_id": "ethereum.0x6666666666666666666666666666666666666666.666",
        "chain": "ethereum",
        "contract_address": "0x6666666666666666666666666666666666666666",
        "token_id": "666",
        "contract": {
          "type": "ERC721",
          "symbol": "SIX"
        },
        "collection": {
          "spam_score": 100
        }
      }
    ]
  })";
  responses[next_url] = json2;
  SetInterceptors(responses);
  auto nft2 = mojom::BlockchainToken::New();
  nft2->chain_id = mojom::kMainnetChainId;
  nft2->contract_address = "0x5555555555555555555555555555555555555555";
  nft2->token_id = "0x22b";  // "555"
  nft2->is_erc20 = false;
  nft2->is_erc721 = true;
  nft2->is_erc1155 = false;
  nft2->is_nft = true;
  nft2->symbol = "FIVE";
  nft2->coin = mojom::CoinType::ETH;
  expected_nfts.push_back(std::move(nft2));
  TestFetchAllNFTsFromSimpleHash(
      "0x0000000000000000000000000000000000000000",
      {mojom::kMainnetChainId, mojom::kOptimismMainnetChainId},
      mojom::CoinType::ETH, expected_nfts);
}

TEST_F(SimpleHashClientUnitTest, FetchNFTsFromSimpleHash) {
  std::map<GURL, std::string> responses;
  GURL url;

  // Test unsupported coin type
  TestFetchNFTsFromSimpleHash("0x0000000000000000000000000000000000000000",
                              {mojom::kMainnetChainId}, mojom::CoinType::FIL,
                              std::nullopt, true, false, {}, std::nullopt);

  // Test invalid URL
  TestFetchNFTsFromSimpleHash("", {mojom::kMainnetChainId},
                              mojom::CoinType::ETH, std::nullopt, true, false,
                              {}, std::nullopt);

  // Non 2xx response yields empty expected_nfts
  SetHTTPRequestTimeoutInterceptor();
  TestFetchNFTsFromSimpleHash("0x0000000000000000000000000000000000000000",
                              {mojom::kMainnetChainId}, mojom::CoinType::ETH,
                              std::nullopt, true, false, {}, std::nullopt);

  // Single NFT fetched without cursor argument
  std::vector<mojom::BlockchainTokenPtr> expected_nfts;
  auto nft1 = mojom::BlockchainToken::New();
  nft1->chain_id = mojom::kPolygonMainnetChainId;
  nft1->contract_address = "0x1111111111111111111111111111111111111111";
  nft1->token_id = "0x1";
  nft1->is_erc721 = true;
  nft1->is_erc1155 = false;
  nft1->is_erc20 = false;
  nft1->is_nft = true;
  nft1->symbol = "ONE";
  nft1->coin = mojom::CoinType::ETH;
  expected_nfts.push_back(std::move(nft1));

  std::string json = R"({
    "next": null,
    "previous": null,
    "nfts": [
      {
        "chain": "polygon",
        "contract_address": "0x1111111111111111111111111111111111111111",
        "token_id": "1",
        "contract": {
          "type": "ERC721",
          "symbol": "ONE"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";

  url = GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "owners?chains=ethereum&wallet_addresses="
      "0x0000000000000000000000000000000000000000");
  responses[url] = json;
  SetInterceptors(responses);

  TestFetchNFTsFromSimpleHash("0x0000000000000000000000000000000000000000",
                              {mojom::kMainnetChainId}, mojom::CoinType::ETH,
                              std::nullopt, true, false, expected_nfts,
                              std::nullopt);

  // Single NFT fetched with cursor argument also returning a cursor
  url = GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "owners?chains=ethereum&wallet_addresses="
      "0x0000000000000000000000000000000000000000&cursor=abc123");
  json = R"({
    "next": null,
    "next_cursor": "def456",
    "previous": null,
    "nfts": [
      {
        "chain": "polygon",
        "contract_address": "0x1111111111111111111111111111111111111111",
        "token_id": "1",
        "contract": {
          "type": "ERC721",
          "symbol": "ONE"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  responses[url] = json;
  SetInterceptors(responses);
  TestFetchNFTsFromSimpleHash("0x0000000000000000000000000000000000000000",
                              {mojom::kMainnetChainId}, mojom::CoinType::ETH,
                              "abc123", true, false, expected_nfts, "def456");

  // Test fetching only spam NFTs
  url = GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "owners?chains=ethereum&wallet_addresses="
      "0x0000000000000000000000000000000000000000");
  std::string json2 = R"({
    "next": null,
    "previous": null,
    "nfts": [
      {
        "chain": "polygon",
        "contract_address": "0x3333333333333333333333333333333333333333",
        "token_id": "3",
        "contract": {
          "type": "ERC721",
          "symbol": "THREE"
        },
        "collection": {
          "spam_score": 100
        }
      },
      {
        "chain": "polygon",
        "contract_address": "0x4444444444444444444444444444444444444444",
        "token_id": "4",
        "contract": {
          "type": "ERC721",
          "symbol": "FOUR"
        },
        "collection": {
          "spam_score": 0
        }
      }
    ]
  })";
  responses[url] = json2;
  SetInterceptors(responses);

  std::vector<mojom::BlockchainTokenPtr> expected_nfts_only_spam;
  auto nft3 = mojom::BlockchainToken::New();
  nft3->chain_id = mojom::kPolygonMainnetChainId;
  nft3->contract_address = "0x3333333333333333333333333333333333333333";
  nft3->token_id = "0x3";
  nft3->is_erc721 = true;
  nft3->is_erc1155 = false;
  nft3->is_erc20 = false;
  nft3->is_nft = true;
  nft3->symbol = "THREE";
  nft3->coin = mojom::CoinType::ETH;
  expected_nfts_only_spam.push_back(std::move(nft3));

  TestFetchNFTsFromSimpleHash("0x0000000000000000000000000000000000000000",
                              {mojom::kMainnetChainId}, mojom::CoinType::ETH,
                              std::nullopt, false, true,
                              expected_nfts_only_spam, std::nullopt);
}

}  // namespace brave_wallet
