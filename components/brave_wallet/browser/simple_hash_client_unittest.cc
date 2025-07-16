/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/simple_hash_client.h"

#include <map>
#include <optional>

#include "base/containers/to_vector.h"
#include "base/strings/string_number_conversions.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "base/test/values_test_util.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/test_utils.h"
#include "services/data_decoder/public/cpp/test_support/in_process_data_decoder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/base/l10n/l10n_util.h"

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
      const std::vector<std::string>& chain_ids_str,
      mojom::CoinType coin,
      const std::vector<mojom::BlockchainTokenPtr>& expected_nfts) {
    auto chain_ids = base::ToVector(chain_ids_str, [&](auto& item) {
      return mojom::ChainId::New(coin, item);
    });
    base::RunLoop run_loop;
    simple_hash_client_->FetchAllNFTsFromSimpleHash(
        account_address, std::move(chain_ids),
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
      const std::vector<std::string>& chain_ids_str,
      mojom::CoinType coin,
      std::optional<std::string> cursor,
      bool skip_spam,
      bool only_spam,
      const std::vector<mojom::BlockchainTokenPtr>& expected_nfts,
      std::optional<std::string> expected_cursor) {
    auto chain_ids = base::ToVector(chain_ids_str, [&](auto& item) {
      return mojom::ChainId::New(coin, item);
    });
    base::RunLoop run_loop;
    simple_hash_client_->FetchNFTsFromSimpleHash(
        account_address, std::move(chain_ids), cursor, skip_spam, only_spam,
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

  void TestFetchSolCompressedNftProofData(
      const std::string& token_address,
      const std::optional<SolCompressedNftProofData>& expected_proof_data) {
    base::RunLoop run_loop;
    simple_hash_client_->FetchSolCompressedNftProofData(
        token_address, base::BindLambdaForTesting(
                           [&](std::optional<SolCompressedNftProofData> proof) {
                             EXPECT_EQ(proof, expected_proof_data);
                             run_loop.Quit();
                           }));
    run_loop.Run();
  }

  void TestGetNfts(
      std::vector<mojom::NftIdentifierPtr> nft_identifiers,
      const std::vector<mojom::BlockchainTokenPtr>& expected_nfts) {
    base::RunLoop run_loop;
    simple_hash_client_->GetNfts(
        std::move(nft_identifiers),
        base::BindLambdaForTesting(
            [&](std::vector<mojom::BlockchainTokenPtr> nfts) {
              ASSERT_EQ(nfts.size(), expected_nfts.size());
              EXPECT_EQ(nfts, expected_nfts);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetNftMetadatas(
      std::vector<mojom::NftIdentifierPtr> nft_identifiers,
      const base::expected<std::vector<mojom::NftMetadataPtr>, std::string>&
          expected) {
    base::RunLoop run_loop;
    simple_hash_client_->GetNftMetadatas(
        std::move(nft_identifiers),
        base::BindLambdaForTesting(
            [&](base::expected<std::vector<mojom::NftMetadataPtr>, std::string>
                    metadatas) {
              ASSERT_EQ(metadatas, expected);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  void TestGetNftBalances(
      const std::string& wallet_address,
      std::vector<mojom::NftIdentifierPtr> nft_identifiers,
      const base::expected<std::vector<uint64_t>, std::string>& expected) {
    base::RunLoop run_loop;
    simple_hash_client_->GetNftBalances(
        wallet_address, std::move(nft_identifiers),
        base::BindLambdaForTesting(
            [&](base::expected<std::vector<uint64_t>, std::string> balances) {
              ASSERT_EQ(balances, expected);
              run_loop.Quit();
            }));
    run_loop.Run();
  }

  network::TestURLLoaderFactory url_loader_factory_;
  scoped_refptr<network::SharedURLLoaderFactory> shared_url_loader_factory_;
  base::test::TaskEnvironment task_environment_;
  std::unique_ptr<SimpleHashClient> simple_hash_client_;
  data_decoder::test::InProcessDataDecoder in_process_data_decoder_;
};

TEST_F(SimpleHashClientUnitTest, GetSimpleHashNftsByWalletUrl) {
  // Empty address yields empty URL
  EXPECT_EQ(
      simple_hash_client_->GetSimpleHashNftsByWalletUrl(
          "", test::MakeVectorFromArgs(EthMainnetChainId()), std::nullopt),
      GURL(""));

  // Empty chains yields empty URL
  EXPECT_EQ(simple_hash_client_->GetSimpleHashNftsByWalletUrl(
                "0x0000000000000000000000000000000000000000", {}, std::nullopt),
            GURL());

  // One valid chain yields correct URL
  EXPECT_EQ(simple_hash_client_->GetSimpleHashNftsByWalletUrl(
                "0x0000000000000000000000000000000000000000",
                test::MakeVectorFromArgs(EthMainnetChainId()), std::nullopt),
            GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
                 "owners?chains=ethereum&wallet_addresses="
                 "0x0000000000000000000000000000000000000000"));

  // Two valid chains yields correct URL
  EXPECT_EQ(simple_hash_client_->GetSimpleHashNftsByWalletUrl(
                "0x0000000000000000000000000000000000000000",
                test::MakeVectorFromArgs(
                    EthMainnetChainId(),
                    mojom::ChainId::New(mojom::CoinType::ETH,
                                        mojom::kOptimismMainnetChainId)),
                std::nullopt),
            GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
                 "owners?chains=ethereum%2Coptimism&wallet_addresses="
                 "0x0000000000000000000000000000000000000000"));

  // One invalid chain yields empty URL
  EXPECT_EQ(
      simple_hash_client_->GetSimpleHashNftsByWalletUrl(
          "0x0000000000000000000000000000000000000000",
          test::MakeVectorFromArgs(mojom::ChainId::New(
              mojom::CoinType::ETH, "chain ID not supported by SimpleHash")),
          std::nullopt),
      GURL());

  // One valid chain with cursor yields correct URL
  std::optional<std::string> cursor = "example_cursor";
  EXPECT_EQ(
      simple_hash_client_->GetSimpleHashNftsByWalletUrl(
          "0x0000000000000000000000000000000000000000",
          test::MakeVectorFromArgs(EthMainnetChainId()), cursor),
      GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
           "owners?chains=ethereum&wallet_addresses="
           "0x0000000000000000000000000000000000000000&cursor=example_cursor"));

  // Two valid chains with cursor yields correct URL
  EXPECT_EQ(
      simple_hash_client_->GetSimpleHashNftsByWalletUrl(
          "0x0000000000000000000000000000000000000000",
          test::MakeVectorFromArgs(
              EthMainnetChainId(),
              mojom::ChainId::New(mojom::CoinType::ETH,
                                  mojom::kOptimismMainnetChainId)),
          cursor),
      GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
           "owners?chains=ethereum%2Coptimism&wallet_addresses="
           "0x0000000000000000000000000000000000000000&cursor=example_cursor"));
}

TEST_F(SimpleHashClientUnitTest, ParseNFTsFromSimpleHash) {
  // Missing 'nfts' key yields nullopt
  std::string json = R"({"foo": "bar"})";
  auto result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), true, false);
  ASSERT_FALSE(result);

  // Dictionary type 'nfts' key yields nullopt
  json = R"({"nfts": {}})";
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), true, false);
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
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), true, false);
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
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), true, false);
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
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), true, false);

  // Valid, 1 ETH NFT
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), true, false);
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
  EXPECT_EQ(result->second[0]->is_compressed, false);
  EXPECT_EQ(result->second[0]->spl_token_program,
            mojom::SPLTokenProgram::kUnsupported);

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
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), true, false);
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
  EXPECT_EQ(result->second[0]->is_compressed, false);
  EXPECT_EQ(result->second[0]->spl_token_program,
            mojom::SPLTokenProgram::kUnsupported);

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
  EXPECT_EQ(result->second[1]->is_compressed, false);
  EXPECT_EQ(result->second[1]->spl_token_program,
            mojom::SPLTokenProgram::kUnsupported);

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
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), true, false);
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

  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), true, false);
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
  EXPECT_EQ(result->second[0]->is_compressed, false);
  EXPECT_EQ(result->second[0]->spl_token_program,
            mojom::SPLTokenProgram::kUnknown);

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
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), true, false);
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
  EXPECT_EQ(result->second[0]->spl_token_program,
            mojom::SPLTokenProgram::kUnknown);

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
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), true, false);
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
  EXPECT_EQ(result->second[0]->spl_token_program,
            mojom::SPLTokenProgram::kUnknown);

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

  // When skip_spam is true and only_spam is false, non spam token should be
  // parsed
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), true, false);
  ASSERT_TRUE(result);
  ASSERT_EQ(result->second.size(), 1u);
  EXPECT_EQ(result->second[0]->contract_address,
            "BHWBJ7XtBqJJbg9SrAUH4moeF8VpJo3WXyDh6vc1qqLG");
  EXPECT_FALSE(result->second[0]->is_spam);

  // When skip_spam is false and only_spam is true, spam token should be parsed
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), false, true);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->second.size(), 1u);
  EXPECT_EQ(result->second[0]->contract_address,
            "AvdAUsR4qgsT5HgyKCVeGjimmyu8xrG3RudFqm5txDDE");
  EXPECT_FALSE(result->second[0]->is_spam);

  // When only_spam is set and skip_spam is set, parsing should fail
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), true, true);
  ASSERT_FALSE(result);

  // When only_spam is false and skip_spam is false, spam and non spam should be
  // parsed
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), false, false);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->second.size(), 2u);
  EXPECT_EQ(result->second[0]->contract_address,
            "AvdAUsR4qgsT5HgyKCVeGjimmyu8xrG3RudFqm5txDDE");
  EXPECT_FALSE(result->second[0]->is_spam);
  EXPECT_EQ(result->second[1]->contract_address,
            "BHWBJ7XtBqJJbg9SrAUH4moeF8VpJo3WXyDh6vc1qqLG");
  EXPECT_FALSE(result->second[1]->is_spam);

  // Compressed SOL NFT
  json = R"({
    "next_cursor": null,
    "next": null,
    "previous": null,
    "nfts": [
      {
        "chain": "solana",
        "contract_address": "6FoSmkL9Z6yoFtTrhsC8Zq4w4PDpsMfGRXSgiR3ri66n",
        "token_id": null,
        "name": "2.0 Jupiter AirDrop",
        "description": "Visit the domain shown in the picture and claim your exclusive voucher jupdrop66.com",
        "image_url": "https://cdn.simplehash.com/assets/663f4be09316c554b420bf869baa82f3081d44abf95f6687f58a4dd99fe8e23e.png",
        "contract": {
          "type": "NonFungible",
          "name": "2.0 Jupiter AirDrop",
          "symbol": "Jup2.0"
        },
        "collection": {
          "spam_score": 100
        },
        "last_sale": null,
        "first_created": {
          "minted_to": "FBG2vwk2tGKHbEWHSxf7rJGDuZ2eHaaNQ8u6c7xGt9Yv",
          "quantity": 1,
          "quantity_string": "1",
          "timestamp": "2024-02-18T16:34:36",
          "block_number": 248974309,
          "transaction": "4n1vvPwnMP7Hrjqek3yqXcVVd4LPtyvum5278x95QkWkrGUxm8SVhH3idtLHeDZndoGg4cpWNq1AmTGTQXhWcaKD",
          "transaction_initiator": "6G9UfJJEgQpNB7rDWoVRHcF93nAShcFu7EwedYkua3PH"
        },
        "rarity": {
          "rank": 2343,
          "score": 1.053,
          "unique_attributes": 0
        },
        "royalty": [
          {
            "source": "metaplex",
            "total_creator_fee_basis_points": 0,
            "recipients": []
          }
        ],
        "extra_metadata": {
          "compression": {
            "compressed": true,
            "merkle_tree": "7eFJyb6UF4hQS7nSQaiy8Xpdq6V7Q1ZRjD3Lze11DZTd",
            "leaf_index": 1316261
          },
          "token_program": "BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY"
        }
      }
    ]
  })";
  result = simple_hash_client_->ParseNFTsFromSimpleHash(
      base::test::ParseJsonDict(json), false, false);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->second.size(), 1u);
  EXPECT_EQ(result->second[0]->contract_address,
            "6FoSmkL9Z6yoFtTrhsC8Zq4w4PDpsMfGRXSgiR3ri66n");
  EXPECT_EQ(result->second[0]->is_compressed, true);
  EXPECT_EQ(result->second[0]->spl_token_program,
            mojom::SPLTokenProgram::kUnknown);
}

TEST_F(SimpleHashClientUnitTest, ParseSolCompressedNftProofData) {
  // Valid JSON data
  std::string json = R"({
  "root": "5bR96ZfMpkDCBQBFvNwdMRizNTp5ZcNEAYq6J3D7mXMR",
  "proof": [
    "ANs5srcJ9fSZpbGmJGXy8M6G3NeNABzK8SshSb9JCwAz",
    "7Kd9DCCFMFrezFznsWAqwA6jtmRRVVHjon5oKVJFffDf",
    "BvSxmwtVL5bx41gnKhpx2hTdYnXdJ1XfetwwHxQPC8Mn",
    "GEtJJVAYjv5mknVVVSjvLmy7BJeQWSdKhbTWdfqLHhpK",
    "VbqjLNCgxCE6Mm9WMTtBxNmthVHqs557AXRRTMhTr4t",
    "3obQ6KPFsC9QfM6g3ZtYC2RbHPfUKn4iBnDecfZoBhbG",
    "DTLQKdFQj8ywDktN1BqR6oe48XGyoSGzAzQgX9QWfnBk",
    "6zZokt6UsXMNEcXPYn3T2LfSaZN6DmZoDwqc3rM16ohu",
    "4aPfGxhmkgrh6Lz82dsi4mdcNC3vZyE1AXiYbJQta4Gw",
    "2AG8n5BwPATab9wWJ2g9XuqXS4xBiQvLVHhn1zX715Ub",
    "JAN9FwHcwqi79Um4MxzrBkTPYEtLHFkUFP8FbnPAFCzc",
    "Ha6247eWxRgGyFCN2NfLbkKMEpLwU1zmkx1QwwRxQ5Ne",
    "6Rt4B2UPizK2gdvmsd8KahazFtc8S5johvGZCUXmHGyV",
    "25wz52GHDo7vX9QSYbUwMd1gi82MUm8sdmAj5jFX8MAH",
    "5W1NH3cKSBdrKeXbd2t8QdwdTU4qTFpSrr1FZyVgHeS8",
    "2XTZ9pTcLXFxGw1hBGrzXMGJrMnvo47sGyLUQwF88SUb",
    "Sia7ffUkzN8xqRHLX4xRdFXzUbVv7LtzRzKDBz8hgDK",
    "4XjrBbzyUWXxXECf173MukGdjHDWQMJ7rs2ojny445my",
    "DqbTjtfiRPHZf2wwmMJ38acyJNTHeiYBsrySSjbMYNiE",
    "2msvGdBzYX2sHifvvr8kJ6YYYvCK2gjjbRZH2tAQ93d5",
    "2XvcBPNUGQSWmyjqYYk9WDFsKLF9oMrnAYxKBJGsPXtw",
    "HSURhkbUwDFSy464A5vNPuPaqe1vWb51YeAf689oprx8",
    "76hjrsKb9iKgHhiY2Np3NYPZaEwnzGcsr6mwyzj4Grj8",
    "6FMzwZu6MxNiBkrE9e6w5fwh925YJEJoRNyQQ9JnrJs3"
  ],
  "merkle_tree": "7eFJyb6UF4hQS7nSQaiy8Xpdq6V7Q1ZRjD3Lze11DZTd",
  "data_hash": "4yfgTevXs3x93pS8tfaqh92y22gAqcRS6Ptt8s6uR3u2",
  "creator_hash": "BSao3oE3zsHmciedhR95HTFyASwrMrwPkcA3xZH9iyzL",
  "leaf_index": "1316261",
  "owner": "FBG2vwk2tGKHbEWHSxf7rJGDuZ2eHaaNQ8u6c7xGt9Yv",
  "delegate": "6G9UfJJEgQpNB7rDWoVRHcF93nAShcFu7EwedYkua3PH",
  "canopy_depth": "0"
})";
  auto result = simple_hash_client_->ParseSolCompressedNftProofData(
      base::test::ParseJsonDict(json));
  ASSERT_TRUE(result);

  EXPECT_EQ(result->root, "5bR96ZfMpkDCBQBFvNwdMRizNTp5ZcNEAYq6J3D7mXMR");
  EXPECT_EQ(result->data_hash, "4yfgTevXs3x93pS8tfaqh92y22gAqcRS6Ptt8s6uR3u2");
  EXPECT_EQ(result->creator_hash,
            "BSao3oE3zsHmciedhR95HTFyASwrMrwPkcA3xZH9iyzL");
  EXPECT_EQ(result->leaf_index, 1316261u);
  EXPECT_EQ(result->owner, "FBG2vwk2tGKHbEWHSxf7rJGDuZ2eHaaNQ8u6c7xGt9Yv");
  ASSERT_EQ(result->proof.size(), 24u);
  EXPECT_EQ(result->proof.front(),
            "ANs5srcJ9fSZpbGmJGXy8M6G3NeNABzK8SshSb9JCwAz");
  EXPECT_EQ(result->proof.back(),
            "6FMzwZu6MxNiBkrE9e6w5fwh925YJEJoRNyQQ9JnrJs3");
  EXPECT_EQ(result->merkle_tree,
            "7eFJyb6UF4hQS7nSQaiy8Xpdq6V7Q1ZRjD3Lze11DZTd");
  EXPECT_EQ(result->canopy_depth, 0u);  // Correct canopy depth

  // JSON with missing required fields yields std::nullopt
  json = R"({
    "data_hash": "79vyLbMksGJdhR8MBRCi73QhxtUxhSdLPQCCkwNpv5MH"
  })";
  result = simple_hash_client_->ParseSolCompressedNftProofData(
      base::test::ParseJsonDict(json));
  ASSERT_FALSE(result);

  // Incorrect data type for `canopy_depth` yields std::nullopt
  json = R"({
    "data_hash": "79vyLbMksGJdhR8MBRCi73QhxtUxhSdLPQCCkwNpv5MH",
    "creator_hash": "55QLBBtrSxGk3VbBwG3RZKSz4cWHxRkTK1BZnDDKXfNv",
    "proof": [
      "6DQNDJuUQjetFLwr9jejENdkMsJEoJz1FFoNehdQYiE4",
      "5GjkHXXejqyJcX1jMnG4sPRf55TuaFzPYAgvwh86buXd"
    ],
    "merkle_tree": "D7kub8uwwptGUyiuRFpHUBPmYc446ocpoWDoopcDhW42",
    "canopy_depth": "twelve"
  })";
  result = simple_hash_client_->ParseSolCompressedNftProofData(
      base::test::ParseJsonDict(json));
  ASSERT_FALSE(result);
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
  nft1->spl_token_program = mojom::SPLTokenProgram::kUnsupported;
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
  nft2->spl_token_program = mojom::SPLTokenProgram::kUnsupported;
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
  nft1->spl_token_program = mojom::SPLTokenProgram::kUnsupported;
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
  nft3->spl_token_program = mojom::SPLTokenProgram::kUnsupported;
  nft3->is_nft = true;
  nft3->symbol = "THREE";
  nft3->coin = mojom::CoinType::ETH;
  expected_nfts_only_spam.push_back(std::move(nft3));

  TestFetchNFTsFromSimpleHash("0x0000000000000000000000000000000000000000",
                              {mojom::kMainnetChainId}, mojom::CoinType::ETH,
                              std::nullopt, false, true,
                              expected_nfts_only_spam, std::nullopt);
}

TEST_F(SimpleHashClientUnitTest, GetNftsUrl) {
  // Empty yields empty URL
  GURL url = SimpleHashClient::GetNftsUrl({});
  EXPECT_EQ(url, GURL());

  // Single Solana NFT
  auto nft_id = mojom::NftIdentifier::New();
  nft_id->chain_id = SolMainnetChainId();
  nft_id->contract_address = "BoSDWCAWmZEM7TQLg2gawt5wnurGyQu7c77tAcbtzfDG";
  nft_id->token_id = "";

  std::vector<mojom::NftIdentifierPtr> nft_ids;
  nft_ids.push_back(std::move(nft_id));
  url = SimpleHashClient::GetNftsUrl(nft_ids);
  EXPECT_EQ(
      url,
      GURL("https://simplehash.wallet.brave.com/api/v0/nfts/assets"
           "?nft_ids=solana.BoSDWCAWmZEM7TQLg2gawt5wnurGyQu7c77tAcbtzfDG"));
  nft_ids.clear();

  // Single Ethereum NFT with non hex token ID yields empty URL
  nft_id = mojom::NftIdentifier::New();
  nft_id->chain_id = EthMainnetChainId();
  nft_id->contract_address = "0x0";
  nft_id->token_id = "78";
  nft_ids.push_back(std::move(nft_id));
  url = SimpleHashClient::GetNftsUrl(nft_ids);
  EXPECT_EQ(url, GURL());
  nft_ids.clear();

  // Single Ethereum NFT.
  nft_id = mojom::NftIdentifier::New();
  nft_id->chain_id = EthMainnetChainId();
  nft_id->contract_address = "0x0";
  nft_id->token_id = "0x1";
  nft_ids.push_back(std::move(nft_id));
  url = SimpleHashClient::GetNftsUrl(nft_ids);
  EXPECT_EQ(url, GURL("https://simplehash.wallet.brave.com/api/v0/nfts/assets"
                      "?nft_ids=ethereum.0x0.1"));
  nft_ids.clear();

  // 75 NFTs takes two calls, 50 and 25.
  for (int i = 0; i < 75; i++) {
    nft_id = mojom::NftIdentifier::New();
    nft_id->chain_id = EthMainnetChainId();
    nft_id->contract_address = "0x" + base::NumberToString(i);
    nft_id->token_id = "0x" + base::NumberToString(i);
    nft_ids.push_back(std::move(nft_id));
  }
  url = SimpleHashClient::GetNftsUrl(nft_ids);
  EXPECT_EQ(
      url,
      GURL("https://simplehash.wallet.brave.com/api/v0/nfts/assets?nft_ids="
           "ethereum.0x0.0%2Cethereum.0x1.1%2Cethereum.0x2.2%2C"
           "ethereum.0x3.3%2Cethereum.0x4.4%2Cethereum.0x5.5%2C"
           "ethereum.0x6.6%2Cethereum.0x7.7%2Cethereum.0x8.8%2C"
           "ethereum.0x9.9%2Cethereum.0x10.16%2Cethereum.0x11.17%2C"
           "ethereum.0x12.18%2Cethereum.0x13.19%2Cethereum.0x14.20%2C"
           "ethereum.0x15.21%2Cethereum.0x16.22%2Cethereum.0x17.23%2C"
           "ethereum.0x18.24%2Cethereum.0x19.25%2Cethereum.0x20.32%2C"
           "ethereum.0x21.33%2Cethereum.0x22.34%2Cethereum.0x23.35%2C"
           "ethereum.0x24.36%2Cethereum.0x25.37%2Cethereum.0x26.38%2C"
           "ethereum.0x27.39%2Cethereum.0x28.40%2Cethereum.0x29.41%2C"
           "ethereum.0x30.48%2Cethereum.0x31.49%2Cethereum.0x32.50%2C"
           "ethereum.0x33.51%2Cethereum.0x34.52%2Cethereum.0x35.53%2C"
           "ethereum.0x36.54%2Cethereum.0x37.55%2Cethereum.0x38.56%2C"
           "ethereum.0x39.57%2Cethereum.0x40.64%2Cethereum.0x41.65%2C"
           "ethereum.0x42.66%2Cethereum.0x43.67%2Cethereum.0x44.68%2C"
           "ethereum.0x45.69%2Cethereum.0x46.70%2Cethereum.0x47.71%2C"
           "ethereum.0x48.72%2Cethereum.0x49.73"));
  nft_ids.clear();

  // Any invalid chain ID yields empty URL
  nft_id = mojom::NftIdentifier::New();
  nft_id->chain_id = EthMainnetChainId();
  nft_id->contract_address = "0x0";
  nft_id->token_id = "0x1";
  nft_ids.push_back(std::move(nft_id));
  url = SimpleHashClient::GetNftsUrl(nft_ids);
  EXPECT_EQ(url, GURL("https://simplehash.wallet.brave.com/api/v0/nfts/"
                      "assets?nft_ids=ethereum.0x0.1"));
  nft_id = mojom::NftIdentifier::New();
  nft_id->chain_id =
      mojom::ChainId::New(mojom::CoinType::ETH, "invalid_chain_id");
  nft_id->contract_address = "0x0";
  nft_id->token_id = "1";
  nft_ids.push_back(std::move(nft_id));
  url = SimpleHashClient::GetNftsUrl(nft_ids);
  EXPECT_EQ(url, GURL());
}

TEST_F(SimpleHashClientUnitTest, GetNfts) {
  // Empty inputs yields no tokens
  std::vector<mojom::NftIdentifierPtr> nft_ids;
  std::vector<mojom::BlockchainTokenPtr> expected_nfts;
  TestGetNfts({}, expected_nfts);

  // Add the chain_id, contract, and token_id from this URL
  auto nft_id = mojom::NftIdentifier::New();
  nft_id->chain_id = SolMainnetChainId();
  nft_id->contract_address = "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR";
  nft_id->token_id = "";
  nft_ids.push_back(std::move(nft_id));

  auto nft_id2 = mojom::NftIdentifier::New();
  nft_id2->chain_id = SolMainnetChainId();
  nft_id2->contract_address = "3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8";
  nft_id2->token_id = "";
  nft_ids.push_back(std::move(nft_id2));

  std::map<GURL, std::string> responses;
  std::string json = R"({
    "nfts": [
      {
        "nft_id": "solana.2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "chain": "solana",
        "contract_address": "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "token_id": null,
        "name": "Common Water Warrior #19",
        "description": "A true gladiator standing with his two back legs, big wings that make him move and attack quickly, and his tail like a big sword that can easily cut-off enemies into slices.",
        "image_url": "https://cdn.simplehash.com/assets/168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png",
        "contract": {
          "type": "NonFungibleEdition",
          "name": "Common Water Warrior #19",
          "symbol": "DRAGON",
          "deployed_by": null,
          "deployed_via_contract": null,
          "owned_by": null,
          "has_multiple_collections": false
        },
        "collection": {
          "collection_id": "2732df34e18c360ccc0cc0809177c70b",
          "name": null,
          "description": null,
          "image_url": "https://lh3.googleusercontent.com/WXQW8GJiTDlucKnaip3NJC_4iFvLCfbQ_Ep9y4D7x-ElE5jOMlKJwcyqD7v27M7yPNiHlIxq9clPqylLlQVoeNfFvmXqboUPhDsS",
          "spam_score": 73
        },
        "last_sale": null,
        "first_created": {},
        "rarity": {
          "rank": null,
          "score": null,
          "unique_attributes": null
        },
        "royalty": [],
        "extra_metadata": {
          "metadata_original_url": "https://nft.dragonwar.io/avatars/dragons/CWTWRDR_1.json"
        }
      },
      {
        "nft_id": "solana.3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8",
        "chain": "solana",
        "contract_address": "3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8",
        "token_id": null,
        "name": "Sneaker #432819057",
        "description": "NFT Sneaker, use it in STEPN to move2earn",
        "previews": {},
        "image_url": "https://cdn.simplehash.com/assets/8ceccddf1868cf1d3860184fab3f084049efecdbaafb4eea43a1e33823c161a1.png",
        "owners": [],
        "contract": {
          "type": "NonFungible",
          "name": "Sneaker #432819057",
          "symbol": null,
          "deployed_by": null,
          "deployed_via_contract": null,
          "owned_by": null,
          "has_multiple_collections": false
        },
        "collection": {
          "collection_id": "34ca10e43844ca82cb9e7ce41b280fba",
          "name": "STEPN",
          "description": "[FYI] We're working with StepN to resolve an issue of being rate limited when fetching NFT metadata from their API, and before we have a resolution, the item-details page for some stepn NFTs won't open.\n---\nSTEPN is the world’s first move2earn NFT mobile game. In STEPN, your steps are worth more than you think – exercising and moving outdoors can now earn anyone tokens anytime, anywhere. STEPN aims to nudge millions into healthier lifestyles and bring them to the Web 3.0 world.",
          "image_url": "https://lh3.googleusercontent.com/2MyUd3epc1SAGOJChg3Pu6GXH-Ip4Q0AcVSUyKCSGMTw6wvPTpOAjntzt6FVg8866LRP2_F5rK4lrNyNDEmg2PwTAtEdZ5j6mB8"
        },
        "extra_metadata": {
          "attributes": [],
          "collection": {
            "name": "Sneaker",
            "family": "STEPN"
          }
        }
      }
    ]
  })";
  GURL url = GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "assets?nft_ids=solana.2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR%"
      "2Csolana.3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8");
  responses[url] = json;
  SetInterceptors(responses);

  // Add the expected NFTs
  auto nft1 = mojom::BlockchainToken::New();
  nft1->chain_id = mojom::kSolanaMainnet;
  nft1->contract_address = "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR";
  nft1->token_id = "";
  nft1->is_erc721 = false;
  nft1->is_erc1155 = false;
  nft1->is_nft = true;
  nft1->symbol = "DRAGON";
  nft1->coin = mojom::CoinType::SOL;
  nft1->name = "Common Water Warrior #19";
  nft1->logo =
      "https://cdn.simplehash.com/assets/"
      "168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png";
  nft1->spl_token_program = mojom::SPLTokenProgram::kUnknown;
  expected_nfts.push_back(std::move(nft1));

  auto nft2 = mojom::BlockchainToken::New();
  nft2->chain_id = mojom::kSolanaMainnet;
  nft2->contract_address = "3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8";
  nft2->token_id = "";
  nft2->is_erc721 = false;
  nft2->is_erc1155 = false;
  nft2->is_nft = true;
  nft2->symbol =
      "";  // Since there's no symbol in the expected, we keep this empty.
  nft2->coin = mojom::CoinType::SOL;
  nft2->name = "Sneaker #432819057";
  nft2->logo =
      "https://cdn.simplehash.com/assets/"
      "8ceccddf1868cf1d3860184fab3f084049efecdbaafb4eea43a1e33823c161a1.png";
  nft2->spl_token_program = mojom::SPLTokenProgram::kUnknown;
  expected_nfts.push_back(std::move(nft2));
  TestGetNfts(std::move(nft_ids), expected_nfts);

  // Test two requests are made if > 50 NFTs are supplied
  nft_ids.clear();
  responses.clear();
  for (int i = 0; i < 75; i++) {
    auto nft_id_0 = mojom::NftIdentifier::New();
    nft_id_0->chain_id = EthMainnetChainId();
    nft_id_0->contract_address = "0x" + base::NumberToString(i);
    nft_id_0->token_id = "0x" + base::NumberToString(i);
    nft_ids.push_back(std::move(nft_id_0));
  }

  responses[GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/assets?nft_ids="
      "ethereum.0x0.0%2Cethereum.0x1.1%2Cethereum.0x2.2%2C"
      "ethereum.0x3.3%2Cethereum.0x4.4%2Cethereum.0x5.5%2C"
      "ethereum.0x6.6%2Cethereum.0x7.7%2Cethereum.0x8.8%2C"
      "ethereum.0x9.9%2Cethereum.0x10.16%2Cethereum.0x11.17%2C"
      "ethereum.0x12.18%2Cethereum.0x13.19%2Cethereum.0x14.20%2C"
      "ethereum.0x15.21%2Cethereum.0x16.22%2Cethereum.0x17.23%2C"
      "ethereum.0x18.24%2Cethereum.0x19.25%2Cethereum.0x20.32%2C"
      "ethereum.0x21.33%2Cethereum.0x22.34%2Cethereum.0x23.35%2C"
      "ethereum.0x24.36%2Cethereum.0x25.37%2Cethereum.0x26.38%2C"
      "ethereum.0x27.39%2Cethereum.0x28.40%2Cethereum.0x29.41%2C"
      "ethereum.0x30.48%2Cethereum.0x31.49%2Cethereum.0x32.50%2C"
      "ethereum.0x33.51%2Cethereum.0x34.52%2Cethereum.0x35.53%2C"
      "ethereum.0x36.54%2Cethereum.0x37.55%2Cethereum.0x38.56%2C"
      "ethereum.0x39.57%2Cethereum.0x40.64%2Cethereum.0x41.65%2C"
      "ethereum.0x42.66%2Cethereum.0x43.67%2Cethereum.0x44.68%2C"
      "ethereum.0x45.69%2Cethereum.0x46.70%2Cethereum.0x47.71%2C"
      "ethereum.0x48.72%2Cethereum.0x49.73")] = "{}";
  responses[GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/assets?nft_ids="
      "ethereum.0x50.80%2Cethereum.0x51.81%2Cethereum.0x52.82%2C"
      "ethereum.0x53.83%2Cethereum.0x54.84%2Cethereum.0x55.85%2C"
      "ethereum.0x56.86%2Cethereum.0x57.87%2Cethereum.0x58.88%2C"
      "ethereum.0x59.89%2Cethereum.0x60.96%2Cethereum.0x61.97%2C"
      "ethereum.0x62.98%2Cethereum.0x63.99%2Cethereum.0x64.100%2C"
      "ethereum.0x65.101%2Cethereum.0x66.102%2Cethereum.0x67.103%2C"
      "ethereum.0x68.104%2Cethereum.0x69.105%2Cethereum.0x70.112%2C"
      "ethereum.0x71.113%2Cethereum.0x72.114%2Cethereum.0x73.115%2C"
      "ethereum.0x74.116")] = json;
  SetInterceptors(responses);
  TestGetNfts(std::move(nft_ids), expected_nfts);
}

TEST_F(SimpleHashClientUnitTest, ParseMetadatas) {
  std::string json;
  std::optional<base::Value> json_value;

  // Ethereum test data. Use all lowercase eth address in response to verify
  // that it is converted to a checksum address.
  json = R"({
    "nfts": [
      {
        "nft_id": "ethereum.0xed5af388653567af2f388e6224dc7c4b3241c544.2767",
        "chain": "ethereum",
        "contract_address": "0xed5af388653567af2f388e6224dc7c4b3241c544",
        "token_id": "2767",
        "name": "Azuki #2767",
        "description": "Azuki is a cute little bean",
        "image_url": "https://cdn.simplehash.com/assets/168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png",
        "extra_metadata": {
          "metadata_original_url": "ipfs://QmZcH4YvBVVRJtdn4RdbaqgspFU8gH6P9vomDpBVpAL3u4/2767",
          "attributes": [
            {
              "trait_type": "Color",
              "value": "Red"
            },
            {
              "trait_type": "Size",
              "value": "Small"
            }
          ]
        },
        "background_color": "#000000",
        "collection": {
          "name": "Azuki"
        }
      }
    ]
  })";
  std::optional<base::flat_map<mojom::NftIdentifierPtr, mojom::NftMetadataPtr>>
      result =
          simple_hash_client_->ParseMetadatas(base::test::ParseJsonDict(json));
  ASSERT_TRUE(result);

  // Verify there is one Ethereum entry.
  EXPECT_EQ(result->size(), 1u);

  mojom::NftIdentifierPtr azuki_identifier = mojom::NftIdentifier::New();
  azuki_identifier->chain_id = EthMainnetChainId();
  // Expect the result to be a checksum address despite HTTP response being all
  // lowercase
  azuki_identifier->contract_address =
      "0xED5AF388653567Af2F388E6224dC7C4b3241C544";
  azuki_identifier->token_id = "0xacf";  // "2767"

  auto it = result->find(azuki_identifier);
  ASSERT_NE(it, result->end());
  EXPECT_EQ(it->second->name, "Azuki #2767");
  EXPECT_EQ(it->second->description, "Azuki is a cute little bean");
  EXPECT_EQ(it->second->image,
            "https://simplehash.wallet-cdn.brave.com/assets/"
            "168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b."
            "png");
  EXPECT_EQ(it->second->image_data, "");
  EXPECT_EQ(it->second->external_url, "");
  ASSERT_EQ(it->second->attributes.size(), 2u);
  EXPECT_EQ(it->second->attributes[0]->trait_type, "Color");
  EXPECT_EQ(it->second->attributes[0]->value, "Red");
  EXPECT_EQ(it->second->attributes[1]->trait_type, "Size");
  EXPECT_EQ(it->second->attributes[1]->value, "Small");
  EXPECT_EQ(it->second->background_color, "#000000");
  EXPECT_EQ(it->second->animation_url, "");
  EXPECT_EQ(it->second->youtube_url, "");
  EXPECT_EQ(it->second->collection, "Azuki");

  // Solana test data
  json = R"({
    "nfts": [
      {
        "nft_id": "solana.2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "chain": "solana",
        "contract_address": "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "token_id": null,
        "name": "Common Water Warrior #19",
        "description": "A true gladiator standing with his two back legs, big wings that make him move and attack quickly, and his tail like a big sword that can easily cut-off enemies into slices.",
        "image_url": "https://cdn.simplehash.com/assets/168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png",
        "extra_metadata": {
          "metadata_original_url": "https://nft.dragonwar.io/avatars/dragons/CWTWRDR_1.json"
        },
        "collection": {
          "name": null
        }
      },
      {
        "nft_id": "solana.3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8",
        "chain": "solana",
        "contract_address": "3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8",
        "token_id": null,
        "extra_metadata": {
          "metadata_original_url": "https://api.stepn.io/run/nftjson/103/118372688129"
        }
      }
    ]
  })";
  result = simple_hash_client_->ParseMetadatas(base::test::ParseJsonDict(json));
  ASSERT_TRUE(result);

  // Verify there are two Solana entries.
  EXPECT_EQ(result->size(), 2u);

  mojom::NftIdentifierPtr warrior_identifier = mojom::NftIdentifier::New();
  warrior_identifier->chain_id = SolMainnetChainId();
  warrior_identifier->contract_address =
      "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR";

  it = result->find(warrior_identifier);
  ASSERT_NE(it, result->end());
  EXPECT_EQ(it->second->name, "Common Water Warrior #19");
  EXPECT_EQ(
      it->second->description,
      "A true gladiator standing with his two back legs, big wings that make "
      "him move and attack quickly, and his tail like a big sword that can "
      "easily cut-off enemies into slices.");
  EXPECT_EQ(it->second->image,
            "https://simplehash.wallet-cdn.brave.com/assets/"
            "168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b."
            "png");
  EXPECT_EQ(it->second->image_data, "");
  EXPECT_EQ(it->second->external_url, "");
  EXPECT_EQ(it->second->attributes.size(), 0u);
  EXPECT_EQ(it->second->background_color, "");
  EXPECT_EQ(it->second->animation_url, "");
  EXPECT_EQ(it->second->youtube_url, "");
  EXPECT_EQ(it->second->collection, "");

  mojom::NftIdentifierPtr ste_nft_identifier = mojom::NftIdentifier::New();
  ste_nft_identifier->chain_id = SolMainnetChainId();
  ste_nft_identifier->contract_address =
      "3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8";

  it = result->find(ste_nft_identifier);
  ASSERT_NE(it, result->end());
  EXPECT_EQ(it->second->name, "");
  EXPECT_EQ(it->second->description, "");
  EXPECT_EQ(it->second->image, "");
  EXPECT_EQ(it->second->image_data, "");
  EXPECT_EQ(it->second->external_url, "");
  EXPECT_EQ(it->second->attributes.size(), 0u);
  EXPECT_EQ(it->second->background_color, "");
  EXPECT_EQ(it->second->animation_url, "");
  EXPECT_EQ(it->second->youtube_url, "");
  EXPECT_EQ(it->second->collection, "");

  // Missing nfts key should return nullopt.
  json = R"({"foo": "bar"})";
  result = simple_hash_client_->ParseMetadatas(base::test::ParseJsonDict(json));
  EXPECT_FALSE(result);

  // NFT missing chain or contract_address should be skipped. The rest should be
  // added.
  json = R"({
    "nfts": [
      {
        "nft_id": "ethereum.0xed5af388653567af2f388e6224dc7c4b3241c544.2767",
        "chain": "ethereum",
        "contract_address": "0xED5AF388653567Af2F388E6224dC7C4b3241C544",
        "token_id": "2767",
        "name": "Azuki #2767",
        "description": "Azuki is a cute little bean",
        "image_url": "https://cdn.simplehash.com/assets/168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png",
        "extra_metadata": {
          "metadata_original_url": "ipfs://QmZcH4YvBVVRJtdn4RdbaqgspFU8gH6P9vomDpBVpAL3u4/2767",
          "attributes": [
            {
              "trait_type": "Color",
              "value": "Red"
            },
            {
              "trait_type": "Size",
              "value": "Small"
            }
          ]
        }
      }
    ]
  })";
  result = simple_hash_client_->ParseMetadatas(base::test::ParseJsonDict(json));
  ASSERT_TRUE(result);
  EXPECT_EQ(result->size(), 1u);

  it = result->find(azuki_identifier);
  ASSERT_NE(it, result->end());
  EXPECT_EQ(it->second->name, "Azuki #2767");
  EXPECT_EQ(it->second->description, "Azuki is a cute little bean");
  EXPECT_EQ(it->second->image,
            "https://simplehash.wallet-cdn.brave.com/assets/"
            "168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b."
            "png");
  EXPECT_EQ(it->second->image_data, "");
  EXPECT_EQ(it->second->external_url, "");
  ASSERT_EQ(it->second->attributes.size(), 2u);
  EXPECT_EQ(it->second->attributes[0]->trait_type, "Color");
  EXPECT_EQ(it->second->attributes[0]->value, "Red");
  EXPECT_EQ(it->second->attributes[1]->trait_type, "Size");
  EXPECT_EQ(it->second->attributes[1]->value, "Small");
  EXPECT_EQ(it->second->background_color, "");
  EXPECT_EQ(it->second->animation_url, "");
  EXPECT_EQ(it->second->youtube_url, "");

  // Test case for NFTs with image url from different CDN
  json = R"({
    "nfts": [
      {
        "nft_id": "ethereum.0xed5af388653567af2f388e6224dc7c4b3241c544.2767",
        "chain": "ethereum",
        "contract_address": "0xED5AF388653567Af2F388E6224dC7C4b3241C544",
        "token_id": "2767",
        "name": "Azuki #2767",
        "description": "Azuki is a cute little bean",
        "image_url": "https://other-cdn.com/assets/img.png",
        "extra_metadata": {
          "metadata_original_url": "ipfs://foo/2767",
          "attributes": [
            {
              "trait_type": "Color",
              "value": "Red"
            },
            {
              "trait_type": "Size",
              "value": "Small"
            }
          ]
        }
      }
    ]
  })";
  result = simple_hash_client_->ParseMetadatas(base::test::ParseJsonDict(json));
  ASSERT_TRUE(result);
  EXPECT_EQ(result->size(), 1u);

  it = result->find(azuki_identifier);
  ASSERT_NE(it, result->end());
  EXPECT_EQ(it->second->image, "https://other-cdn.com/assets/img.png");
  EXPECT_EQ(it->second->image_data, "");
  EXPECT_EQ(it->second->external_url, "");
  ASSERT_EQ(it->second->attributes.size(), 2u);
  EXPECT_EQ(it->second->attributes[0]->trait_type, "Color");
  EXPECT_EQ(it->second->attributes[0]->value, "Red");
  EXPECT_EQ(it->second->attributes[1]->trait_type, "Size");
  EXPECT_EQ(it->second->attributes[1]->value, "Small");
}

TEST_F(SimpleHashClientUnitTest, GetNftMetadatas) {
  // If there are no NFTs, an invalid parameters error is returned.
  std::vector<mojom::NftIdentifierPtr> nft_identifiers;
  std::vector<mojom::NftMetadataPtr> expected_metadatas;
  TestGetNftMetadatas(std::move(nft_identifiers),
                      base::unexpected(l10n_util::GetStringUTF8(
                          IDS_WALLET_INVALID_PARAMETERS)));
  nft_identifiers = std::vector<mojom::NftIdentifierPtr>();

  // If there are > 50 NFTs, an invalid parameters error is returned.
  for (int i = 0; i < 75; i++) {
    auto nft_identifier = mojom::NftIdentifier::New();
    nft_identifier->chain_id = EthMainnetChainId();
    nft_identifier->contract_address =
        "0xED5AF388653567Af2F388E6224dC7C4b3241C544";
    nft_identifier->token_id = "0x" + base::NumberToString(i);
    nft_identifiers.push_back(std::move(nft_identifier));
  }
  expected_metadatas.clear();
  TestGetNftMetadatas(std::move(nft_identifiers),
                      base::unexpected(l10n_util::GetStringUTF8(
                          IDS_WALLET_INVALID_PARAMETERS)));

  std::string json = R"({
    "nfts": [
      {
        "nft_id": "ethereum.0xed5af388653567af2f388e6224dc7c4b3241c544.2767",
        "chain": "ethereum",
        "contract_address": "0xED5AF388653567Af2F388E6224dC7C4b3241C544",
        "token_id": "2767",
        "name": "Azuki #2767",
        "description": "Azuki is a cute little bean",
        "image_url": "https://cdn.simplehash.com/assets/168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png",
        "extra_metadata": {
          "metadata_original_url": "ipfs://QmZcH4YvBVVRJtdn4RdbaqgspFU8gH6P9vomDpBVpAL3u4/2767",
          "attributes": [
            {
              "trait_type": "Color",
              "value": "Red"
            },
            {
              "trait_type": "Size",
              "value": "Small"
            }
          ]
        }
      },
      {
        "nft_id": "solana.2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "chain": "solana",
        "contract_address": "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "token_id": null,
        "name": "Common Water Warrior #19",
        "description": "A true gladiator",
        "image_url": "https://cdn.simplehash.com/assets/168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png",
        "extra_metadata": {
          "metadata_original_url": "https://nft.dragonwar.io/avatars/dragons/CWTWRDR_1.json"
        }
      },
      {
        "nft_id": "solana.3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8",
        "chain": "solana",
        "contract_address": "3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8",
        "token_id": null,
        "name": "Sneaker #432819057",
        "description": "A sneaker",
        "image_url": "https://cdn.simplehash.com/assets/3.png",
        "extra_metadata": {
          "attributes": [
            {
              "trait_type": "Color",
              "value": "Blue"
            },
            {
              "trait_type": "Size",
              "value": "Small"
            }
          ]
        },
        "external_url": "https://nft.dragonwar.io/avatars/dragons/CWTWRDR_1.json",
        "background_color": "#000000",
        "animation_url": null
      }
    ]
  })";

  // Add the chain_id, contract, and token_id from simple hash response
  nft_identifiers.clear();
  auto nft_identifier1 = mojom::NftIdentifier::New();
  nft_identifier1->chain_id = SolMainnetChainId();
  nft_identifier1->contract_address =
      "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR";
  nft_identifier1->token_id = "";
  nft_identifiers.push_back(std::move(nft_identifier1));

  auto nft_identifier2 = mojom::NftIdentifier::New();
  nft_identifier2->chain_id = SolMainnetChainId();
  nft_identifier2->contract_address =
      "3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8";
  nft_identifier2->token_id = "";
  nft_identifiers.push_back(std::move(nft_identifier2));

  std::map<GURL, std::string> responses;
  responses[GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "assets?nft_ids=solana.2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR%"
      "2Csolana.3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8")] = json;

  // Add the expected metadatas
  mojom::NftMetadataPtr metadata1 = mojom::NftMetadata::New();
  metadata1->name = "Common Water Warrior #19";
  metadata1->description = "A true gladiator";
  metadata1->image =
      "https://simplehash.wallet-cdn.brave.com/assets/"
      "168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png";
  metadata1->image_data = "";
  metadata1->external_url = "";
  metadata1->background_color = "";
  metadata1->animation_url = "";
  metadata1->youtube_url = "";

  mojom::NftMetadataPtr metadata2 = mojom::NftMetadata::New();
  metadata2->name = "Sneaker #432819057";
  metadata2->description = "A sneaker";
  metadata2->image = "https://simplehash.wallet-cdn.brave.com/assets/3.png";
  metadata2->image_data = "";
  metadata2->external_url =
      "https://nft.dragonwar.io/avatars/dragons/CWTWRDR_1.json";
  metadata2->background_color = "#000000";
  metadata2->animation_url = "";
  metadata2->youtube_url = "";
  mojom::NftAttributePtr attribute1 = mojom::NftAttribute::New();
  attribute1->trait_type = "Color";
  attribute1->value = "Blue";
  metadata2->attributes.push_back(std::move(attribute1));
  mojom::NftAttributePtr attribute2 = mojom::NftAttribute::New();
  attribute2->trait_type = "Size";
  attribute2->value = "Small";
  metadata2->attributes.push_back(std::move(attribute2));

  expected_metadatas.push_back(std::move(metadata1));
  expected_metadatas.push_back(std::move(metadata2));
  SetInterceptors(responses);
  TestGetNftMetadatas(std::move(nft_identifiers),
                      base::ok(std::move(expected_metadatas)));

  // Test case for duplicate NFT identifiers
  nft_identifiers = std::vector<mojom::NftIdentifierPtr>();
  expected_metadatas = std::vector<mojom::NftMetadataPtr>();

  // Add two identical NFT identifiers
  auto duplicate_nft_identifier1 = mojom::NftIdentifier::New();
  duplicate_nft_identifier1->chain_id = SolMainnetChainId();
  duplicate_nft_identifier1->contract_address =
      "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR";
  duplicate_nft_identifier1->token_id = "";
  nft_identifiers.push_back(std::move(duplicate_nft_identifier1));

  auto duplicate_nft_identifier2 = mojom::NftIdentifier::New();
  duplicate_nft_identifier2->chain_id = SolMainnetChainId();
  duplicate_nft_identifier2->contract_address =
      "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR";
  duplicate_nft_identifier2->token_id = "";
  nft_identifiers.push_back(std::move(duplicate_nft_identifier2));

  // Create JSON response for duplicate NFTs (response will contain only one
  // entry)
  std::string duplicate_json = R"({
    "nfts": [
      {
        "nft_id": "solana.2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "chain": "solana",
        "contract_address": "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "token_id": null,
        "name": "Common Water Warrior #19",
        "description": "A true gladiator",
        "image_url": "https://cdn.simplehash.com/assets/168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png",
        "extra_metadata": {
          "metadata_original_url": "https://nft.dragonwar.io/avatars/dragons/CWTWRDR_1.json"
        }
      }
    ]
  })";

  // Set up expected metadata for the duplicate NFT
  auto duplicate_metadata = mojom::NftMetadata::New();
  duplicate_metadata->name = "Common Water Warrior #19";
  duplicate_metadata->description = "A true gladiator";
  duplicate_metadata->image =
      "https://simplehash.wallet-cdn.brave.com/assets/"
      "168e33bbf5276f717d8d190810ab93b4992ac8681054c1811f8248fe7636b54b.png";
  duplicate_metadata->image_data = "";
  duplicate_metadata->external_url = "";
  duplicate_metadata->background_color = "";
  duplicate_metadata->animation_url = "";
  duplicate_metadata->youtube_url = "";

  // Add the same metadata twice since we expect the API to return the same data
  // for both requests
  expected_metadatas.push_back(duplicate_metadata.Clone());
  expected_metadatas.push_back(std::move(duplicate_metadata));

  // Set up the response interceptor for the duplicate NFT request
  responses.clear();
  responses[GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "assets?nft_ids=solana.2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR%"
      "2Csolana.2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR")] =
      duplicate_json;

  SetInterceptors(responses);
  TestGetNftMetadatas(std::move(nft_identifiers),
                      base::ok(std::move(expected_metadatas)));
}

TEST_F(SimpleHashClientUnitTest, GetNftBalances) {
  std::string wallet_address = "0x123";
  std::vector<mojom::NftIdentifierPtr> nft_identifiers;
  TestGetNftBalances(wallet_address, std::move(nft_identifiers),
                     base::unexpected(l10n_util::GetStringUTF8(
                         IDS_WALLET_INVALID_PARAMETERS)));
  nft_identifiers = std::vector<mojom::NftIdentifierPtr>();

  // More than 50 NFTs yields no balances
  for (int i = 0; i < 75; i++) {
    auto nft_identifier = mojom::NftIdentifier::New();
    nft_identifier->chain_id = EthMainnetChainId();
    nft_identifier->contract_address = "0x" + base::NumberToString(i);
    nft_identifier->token_id = "0x" + base::NumberToString(i);
    nft_identifiers.push_back(std::move(nft_identifier));
  }
  TestGetNftBalances(wallet_address, std::move(nft_identifiers),
                     base::unexpected(l10n_util::GetStringUTF8(
                         IDS_WALLET_INVALID_PARAMETERS)));
  nft_identifiers = std::vector<mojom::NftIdentifierPtr>();

  // Response includes two NFTs, wallet address is included in only one of them
  std::string json = R"({
    "nfts": [
      {
        "nft_id": "solana.3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8",
        "chain": "solana",
        "contract_address": "3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8",
        "token_id": null,
        "name": "Sneaker #432819057",
        "owners": [
          {
            "owner_address": "0x123",
            "quantity": 999
          },
          {
            "owner_address": "0x456",
            "quantity": 2
          }
        ]
      },
      {
        "nft_id": "solana.2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "chain": "solana",
        "contract_address": "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "token_id": null,
        "name": "Common Water Warrior #19",
        "owners": [
          {
            "owner_address": "0x456",
            "quantity": 3
          }
        ]
      }
    ]
  })";

  // Add the chain_id, contract, and token_id from simple hash response
  auto nft_identifier1 = mojom::NftIdentifier::New();
  nft_identifier1->chain_id = SolMainnetChainId();
  nft_identifier1->contract_address =
      "3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8";
  nft_identifier1->token_id = "";
  nft_identifiers.push_back(std::move(nft_identifier1));

  auto nft_identifier2 = mojom::NftIdentifier::New();
  nft_identifier2->chain_id = SolMainnetChainId();
  nft_identifier2->contract_address =
      "2izbbrgnlveezh6jdsansto66s2uxx7dtchvwku8oisr";
  nft_identifier2->token_id = "";
  nft_identifiers.push_back(std::move(nft_identifier2));

  std::map<GURL, std::string> responses;
  responses[GURL(
      "https://simplehash.wallet.brave.com/api/v0/nfts/"
      "assets?nft_ids=solana.3knghmwnuaMxkiuqXrqzjL7gLDuRw6DkkZcW7F4mvkK8%"
      "2Csolana.2izbbrgnlveezh6jdsansto66s2uxx7dtchvwku8oisr")] = json;

  // Add the expected balances
  std::vector<uint64_t> expected_balances;
  expected_balances.push_back(999);
  expected_balances.push_back(0);
  SetInterceptors(responses);
  TestGetNftBalances(wallet_address, std::move(nft_identifiers),
                     base::ok(expected_balances));
}

TEST_F(SimpleHashClientUnitTest, ParseBalances) {
  std::string json;

  // JSON missing NFT key should return nullopt.
  json = R"({"foo": "bar"})";
  std::optional<base::flat_map<mojom::NftIdentifierPtr,
                               base::flat_map<std::string, uint64_t>>>
      result =
          simple_hash_client_->ParseBalances(base::test::ParseJsonDict(json));
  EXPECT_FALSE(result);

  // Ethereum test data. Use all uppercase case address to verify that it is
  // converted to a checksum address.
  json = R"({
    "nfts": [
      {
        "nft_id": "ethereum.0xed5af388653567af2f388e6224dc7c4b3241c544.2767",
        "chain": "ethereum",
        "contract_address": "0xED5AF388653567AF2F388E6224DC7C4B3241C544",
        "token_id": "2767",
        "name": "Azuki #2767",
        "owners": [
          {
            "owner_address": "0x123",
            "quantity": "1"
          },
          {
            "owner_address": "0x456",
            "quantity": "2"
          }
        ]
      }
    ]
  })";

  auto owners =
      simple_hash_client_->ParseBalances(base::test::ParseJsonDict(json));
  ASSERT_TRUE(owners);

  // Verify there is one Ethereum entry.
  EXPECT_EQ(owners->size(), 1u);

  mojom::NftIdentifierPtr azuki_identifier = mojom::NftIdentifier::New();
  azuki_identifier->chain_id = EthMainnetChainId();
  azuki_identifier->contract_address =
      "0xED5AF388653567Af2F388E6224dC7C4b3241C544";  // Checksum address
  azuki_identifier->token_id = "0xacf";              // "2767"

  auto it = owners->find(azuki_identifier);
  ASSERT_NE(it, owners->end());
  EXPECT_EQ(it->second.size(), 2u);
  EXPECT_EQ(it->second["0x123"], 1u);
  EXPECT_EQ(it->second["0x456"], 2u);

  // Solana test data
  json = R"({
    "nfts": [
      {
        "nft_id": "solana.2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "chain": "solana",
        "contract_address": "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "token_id": null,
        "name": "Common Water Warrior #19",
        "extra_metadata": {
          "metadata_original_url": "https://nft.dragonwar.io/avatars/dragons/CWTWRDR_1.json"
        },
        "owners": [
          {
            "owner_address": "0x123",
            "quantity": "3"
          }
        ]
      }
    ]
  })";

  owners = simple_hash_client_->ParseBalances(base::test::ParseJsonDict(json));
  ASSERT_TRUE(owners);

  // Verify there is one Solana entry.
  EXPECT_EQ(owners->size(), 1u);

  mojom::NftIdentifierPtr warrior_identifier = mojom::NftIdentifier::New();
  warrior_identifier->chain_id = SolMainnetChainId();
  warrior_identifier->contract_address =
      "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR";

  it = owners->find(warrior_identifier);
  ASSERT_NE(it, owners->end());
  EXPECT_EQ(it->second.size(), 1u);
  EXPECT_EQ(it->second["0x123"], 3u);

  // NFT missing owners key should be skipped, but the rest should be added.
  json = R"({
    "nfts": [
      {
        "nft_id": "ethereum.0xed5af388653567af2f388e6224dc7c4b3241c544.2767",
        "chain": "ethereum",
        "contract_address": "0xED5AF388653567Af2F388E6224dC7C4b3241C544",
        "token_id": "2767",
        "name": "Azuki #2767"
      },
      {
        "nft_id": "solana.2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "chain": "solana",
        "contract_address": "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR",
        "token_id": null,
        "name": "Common Water Warrior #19",
        "owners": [
          {
            "owner_address": "0x123",
            "quantity": "3"
          }
        ]
      }
    ]
  })";

  owners = simple_hash_client_->ParseBalances(base::test::ParseJsonDict(json));
  ASSERT_TRUE(owners);
  EXPECT_EQ(owners->size(), 1u);
  it = owners->find(warrior_identifier);
  ASSERT_NE(it, owners->end());
  EXPECT_EQ(it->second.size(), 1u);
  EXPECT_EQ(it->second["0x123"], 3u);

  // NFT missing owner_address key should be skipped, but the rest should be
  // added.
  json = R"({
    "nfts": [
      {
        "nft_id": "ethereum.0.ed5af388653567af2f388e6224dc7c4b3241c544.2767",
        "chain": "ethereum",
        "contract_address": "0xED5AF388653567Af2F388E6224dC7C4b3241C544",
        "token_id": "2767",
        "name": "Azuki #2767",
        "owners": [
          {
            "quantity": "1"
          },
          {
            "owner_address": "0x456",
            "quantity": "2"
          }
        ]
      }
    ]
  })";

  owners = simple_hash_client_->ParseBalances(base::test::ParseJsonDict(json));
  ASSERT_TRUE(owners);
  EXPECT_EQ(owners->size(), 1u);

  it = owners->find(azuki_identifier);
  ASSERT_NE(it, owners->end());
  EXPECT_EQ(it->second.size(), 1u);
  EXPECT_EQ(it->second["0x456"], 2u);
}

TEST_F(SimpleHashClientUnitTest, FetchSolCompressedNftProofData) {
  // HTTP timeout should return nullopt.
  std::string token_address = "2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR";
  SetHTTPRequestTimeoutInterceptor();
  TestFetchSolCompressedNftProofData(token_address, std::nullopt);

  // Valid JSON response returns the expected proof data.
  std::string json = R"({
    "root": "5bR96ZfMpkDCBQBFvNwdMRizNTp5ZcNEAYq6J3D7mXMR",
    "proof": [
      "ANs5srcJ9fSZpbGmJGXy8M6G3NeNABzK8SshSb9JCwAz",
      "7Kd9DCCFMFrezFznsWAqwA6jtmRRVVHjon5oKVJFffDf",
      "BvSxmwtVL5bx41gnKhpx2hTdYnXdJ1XfetwwHxQPC8Mn",
      "GEtJJVAYjv5mknVVVSjvLmy7BJeQWSdKhbTWdfqLHhpK",
      "VbqjLNCgxCE6Mm9WMTtBxNmthVHqs557AXRRTMhTr4t",
      "3obQ6KPFsC9QfM6g3ZtYC2RbHPfUKn4iBnDecfZoBhbG",
      "DTLQKdFQj8ywDktN1BqR6oe48XGyoSGzAzQgX9QWfnBk",
      "6zZokt6UsXMNEcXPYn3T2LfSaZN6DmZoDwqc3rM16ohu",
      "4aPfGxhmkgrh6Lz82dsi4mdcNC3vZyE1AXiYbJQta4Gw",
      "2AG8n5BwPATab9wWJ2g9XuqXS4xBiQvLVHhn1zX715Ub",
      "JAN9FwHcwqi79Um4MxzrBkTPYEtLHFkUFP8FbnPAFCzc",
      "Ha6247eWxRgGyFCN2NfLbkKMEpLwU1zmkx1QwwRxQ5Ne",
      "6Rt4B2UPizK2gdvmsd8KahazFtc8S5johvGZCUXmHGyV",
      "25wz52GHDo7vX9QSYbUwMd1gi82MUm8sdmAj5jFX8MAH",
      "5W1NH3cKSBdrKeXbd2t8QdwdTU4qTFpSrr1FZyVgHeS8",
      "2XTZ9pTcLXFxGw1hBGrzXMGJrMnvo47sGyLUQwF88SUb",
      "Sia7ffUkzN8xqRHLX4xRdFXzUbVv7LtzRzKDBz8hgDK",
      "4XjrBbzyUWXxXECf173MukGdjHDWQMJ7rs2ojny445my",
      "DqbTjtfiRPHZf2wwmMJ38acyJNTHeiYBsrySSjbMYNiE",
      "2msvGdBzYX2sHifvvr8kJ6YYYvCK2gjjbRZH2tAQ93d5",
      "2XvcBPNUGQSWmyjqYYk9WDFsKLF9oMrnAYxKBJGsPXtw",
      "HSURhkbUwDFSy464A5vNPuPaqe1vWb51YeAf689oprx8",
      "76hjrsKb9iKgHhiY2Np3NYPZaEwnzGcsr6mwyzj4Grj8",
      "6FMzwZu6MxNiBkrE9e6w5fwh925YJEJoRNyQQ9JnrJs3"
    ],
    "merkle_tree": "7eFJyb6UF4hQS7nSQaiy8Xpdq6V7Q1ZRjD3Lze11DZTd",
    "data_hash": "4yfgTevXs3x93pS8tfaqh92y22gAqcRS6Ptt8s6uR3u2",
    "creator_hash": "BSao3oE3zsHmciedhR95HTFyASwrMrwPkcA3xZH9iyzL",
    "leaf_index": 1316261,
    "owner": "FBG2vwk2tGKHbEWHSxf7rJGDuZ2eHaaNQ8u6c7xGt9Yv",
    "delegate": "6G9UfJJEgQpNB7rDWoVRHcF93nAShcFu7EwedYkua3PH",
    "canopy_depth": 0
  })";
  SetInterceptors(
      {{GURL("https://simplehash.wallet.brave.com/api/v0/nfts/proof/"
             "solana/2iZBbRGnLVEEZH6JDsaNsTo66s2uxx7DTchVWKU8oisR"),
        json}});

  // Create expected SolCompressedNftProofData
  SolCompressedNftProofData expected_proof_data;
  expected_proof_data.root = "5bR96ZfMpkDCBQBFvNwdMRizNTp5ZcNEAYq6J3D7mXMR";
  expected_proof_data.proof = {"ANs5srcJ9fSZpbGmJGXy8M6G3NeNABzK8SshSb9JCwAz",
                               "7Kd9DCCFMFrezFznsWAqwA6jtmRRVVHjon5oKVJFffDf",
                               "BvSxmwtVL5bx41gnKhpx2hTdYnXdJ1XfetwwHxQPC8Mn",
                               "GEtJJVAYjv5mknVVVSjvLmy7BJeQWSdKhbTWdfqLHhpK",
                               "VbqjLNCgxCE6Mm9WMTtBxNmthVHqs557AXRRTMhTr4t",
                               "3obQ6KPFsC9QfM6g3ZtYC2RbHPfUKn4iBnDecfZoBhbG",
                               "DTLQKdFQj8ywDktN1BqR6oe48XGyoSGzAzQgX9QWfnBk",
                               "6zZokt6UsXMNEcXPYn3T2LfSaZN6DmZoDwqc3rM16ohu",
                               "4aPfGxhmkgrh6Lz82dsi4mdcNC3vZyE1AXiYbJQta4Gw",
                               "2AG8n5BwPATab9wWJ2g9XuqXS4xBiQvLVHhn1zX715Ub",
                               "JAN9FwHcwqi79Um4MxzrBkTPYEtLHFkUFP8FbnPAFCzc",
                               "Ha6247eWxRgGyFCN2NfLbkKMEpLwU1zmkx1QwwRxQ5Ne",
                               "6Rt4B2UPizK2gdvmsd8KahazFtc8S5johvGZCUXmHGyV",
                               "25wz52GHDo7vX9QSYbUwMd1gi82MUm8sdmAj5jFX8MAH",
                               "5W1NH3cKSBdrKeXbd2t8QdwdTU4qTFpSrr1FZyVgHeS8",
                               "2XTZ9pTcLXFxGw1hBGrzXMGJrMnvo47sGyLUQwF88SUb",
                               "Sia7ffUkzN8xqRHLX4xRdFXzUbVv7LtzRzKDBz8hgDK",
                               "4XjrBbzyUWXxXECf173MukGdjHDWQMJ7rs2ojny445my",
                               "DqbTjtfiRPHZf2wwmMJ38acyJNTHeiYBsrySSjbMYNiE",
                               "2msvGdBzYX2sHifvvr8kJ6YYYvCK2gjjbRZH2tAQ93d5",
                               "2XvcBPNUGQSWmyjqYYk9WDFsKLF9oMrnAYxKBJGsPXtw",
                               "HSURhkbUwDFSy464A5vNPuPaqe1vWb51YeAf689oprx8",
                               "76hjrsKb9iKgHhiY2Np3NYPZaEwnzGcsr6mwyzj4Grj8",
                               "6FMzwZu6MxNiBkrE9e6w5fwh925YJEJoRNyQQ9JnrJs3"};
  expected_proof_data.merkle_tree =
      "7eFJyb6UF4hQS7nSQaiy8Xpdq6V7Q1ZRjD3Lze11DZTd";
  expected_proof_data.data_hash =
      "4yfgTevXs3x93pS8tfaqh92y22gAqcRS6Ptt8s6uR3u2";
  expected_proof_data.creator_hash =
      "BSao3oE3zsHmciedhR95HTFyASwrMrwPkcA3xZH9iyzL";
  expected_proof_data.leaf_index = 1316261;
  expected_proof_data.owner = "FBG2vwk2tGKHbEWHSxf7rJGDuZ2eHaaNQ8u6c7xGt9Yv";
  expected_proof_data.delegate = "6G9UfJJEgQpNB7rDWoVRHcF93nAShcFu7EwedYkua3PH";
  expected_proof_data.canopy_depth = 0;
  TestFetchSolCompressedNftProofData(token_address, expected_proof_data);
}

}  // namespace brave_wallet
