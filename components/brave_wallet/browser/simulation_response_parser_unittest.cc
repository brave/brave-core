/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/simulation_response_parser.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/strings/stringprintf.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;

namespace brave_wallet {

// Example from https://docs.blowfish.xyz/reference/scan-transactions-evm
TEST(SimulationResponseParserUnitTest, ParseEvmSwapSynForUsdc) {
  std::string json(R"(
    {
      "requestId":"e8cd35ce-f743-4ef2-8e94-f26857744db7",
      "action":"NONE",
      "warnings":[],
      "simulationResults":{
        "aggregated":{
          "expectedStateChanges":{
            "0x06924592cdf28acd3c1d23c37875c6c6a667bdf7":[
              {
                "humanReadableDiff":"Send 5523.75078 SYN",
                "rawInfo":{
                  "kind":"ERC20_TRANSFER",
                  "data":{
                    "amount":{
                      "before":"5523750784922050828352",
                      "after":"0"
                    },
                    "counterparty":{
                      "kind":"ACCOUNT",
                      "address":"0x4a86c01d67965f8cb3d0aaa2c655705e64097c31"
                    },
                    "asset":{
                      "address":"0x0f2d719407fdbeff09d87557abb7232601fd9f29",
                      "symbol":"SYN",
                      "name":"Synapse",
                      "decimals":"18",
                      "verified":true,
                      "lists":[
                        "COINGECKO",
                        "ONE_INCH",
                        "UNISWAP"
                      ],
                      "imageUrl":"https://syn.png",
                      "price":{
                        "source":"Coingecko",
                        "updatedAt":"1689251218",
                        "dollarValuePerToken":"0.645586"
                      }
                    }
                  }
                }
              },
              {
                "humanReadableDiff":"Receive 7975.46196 USDC",
                "rawInfo":{
                  "kind":"ERC20_TRANSFER",
                  "data":{
                    "amount":{
                      "before":"0",
                      "after":"7975461958"
                    },
                    "counterparty":{
                      "kind":"ACCOUNT",
                      "address":"0x397ff1542f962076d0bfe58ea045ffa2d347aca0"
                    },
                    "asset":{
                      "address":"0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48",
                      "symbol":"USDC",
                      "name":"USDCoin",
                      "decimals":"6",
                      "verified":true,
                      "lists":[
                        "COINGECKO",
                        "ZERION",
                        "ONE_INCH",
                        "UNISWAP",
                        "MY_CRYPTO_API",
                        "KLEROS_TOKENS"
                      ],
                      "imageUrl":"https://usdc.png",
                      "price":{
                        "source":"Coingecko",
                        "updatedAt":"1689251215",
                        "dollarValuePerToken":"0.999694"
                      }
                    }
                  }
                }
              },
              {
                "humanReadableDiff":"Approve to transfer any amount of your SYN",
                "rawInfo":{
                  "kind":"ERC20_APPROVAL",
                  "data":{
                    "spender":{
                      "kind":"ACCOUNT",
                      "address":"0xd9e1ce17f2641f24ae83637ab66a2cca9c378b9f"
                    },
                    "amount":{
                      "before":"115792089237316195423570985008687907853269984665640564039457584007913129639935",
                      "after":"115792089237316195423570985008687907853269984665640564033933833222991078811583"
                    },
                    "owner":{
                      "kind":"ACCOUNT",
                      "address":"0x06924592cdf28acd3c1d23c37875c6c6a667bdf7"
                    },
                    "asset":{
                      "address":"0x0f2d719407fdbeff09d87557abb7232601fd9f29",
                      "symbol":"SYN",
                      "name":"Synapse",
                      "decimals":"18",
                      "verified":true,
                      "lists":[
                        "COINGECKO",
                        "ONE_INCH",
                        "UNISWAP"
                      ],
                      "imageUrl":"https://syn.png",
                      "price":{
                        "source":"Coingecko",
                        "updatedAt":"1689251218",
                        "dollarValuePerToken":"0.645586"
                      }
                    }
                  }
                }
              }
            ]
          },
          "error":null,
          "userAccount":"0x06924592cdf28acd3c1d23c37875c6c6a667bdf7"
        }
      }
    }
  )");
  auto simulation_response = evm::ParseSimulationResponse(
      ParseJson(json), "0x06924592cdf28acd3c1d23c37875c6c6a667bdf7");
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kNone);
  EXPECT_EQ(simulation_response->warnings.size(), 0u);
  EXPECT_FALSE(simulation_response->error);
  ASSERT_EQ(simulation_response->expected_state_changes.size(), 3u);

  const auto& state_change_0 =
      simulation_response->expected_state_changes.at(0);

  EXPECT_EQ(state_change_0->human_readable_diff, "Send 5523.75078 SYN");
  EXPECT_EQ(state_change_0->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kErc20Transfer);
  ASSERT_TRUE(state_change_0->raw_info->data->is_erc20_transfer_data());
  const auto& state_change_0_raw_info =
      state_change_0->raw_info->data->get_erc20_transfer_data();
  EXPECT_EQ(state_change_0_raw_info->amount->before, "5523750784922050828352");
  EXPECT_EQ(state_change_0_raw_info->amount->after, "0");
  ASSERT_TRUE(state_change_0_raw_info->counterparty);
  EXPECT_EQ(state_change_0_raw_info->counterparty->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_0_raw_info->counterparty->address,
            "0x4a86c01d67965f8cb3d0aaa2c655705e64097c31");
  ASSERT_TRUE(state_change_0_raw_info->asset);
  EXPECT_EQ(state_change_0_raw_info->asset->address,
            "0x0f2d719407fdbeff09d87557abb7232601fd9f29");
  EXPECT_EQ(state_change_0_raw_info->asset->symbol, "SYN");
  EXPECT_EQ(state_change_0_raw_info->asset->name, "Synapse");
  EXPECT_EQ(state_change_0_raw_info->asset->decimals, 18);
  EXPECT_TRUE(state_change_0_raw_info->asset->verified);
  ASSERT_EQ(state_change_0_raw_info->asset->lists.size(), 3u);
  EXPECT_EQ(state_change_0_raw_info->asset->lists.at(0), "COINGECKO");
  EXPECT_EQ(state_change_0_raw_info->asset->lists.at(1), "ONE_INCH");
  EXPECT_EQ(state_change_0_raw_info->asset->lists.at(2), "UNISWAP");
  EXPECT_EQ(state_change_0_raw_info->asset->image_url, "https://syn.png");
  EXPECT_EQ(state_change_0_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);
  EXPECT_EQ(state_change_0_raw_info->asset->price->last_updated_at,
            "1689251218");
  EXPECT_EQ(state_change_0_raw_info->asset->price->dollar_value_per_token,
            "0.645586");

  const auto& state_change_1 =
      simulation_response->expected_state_changes.at(1);
  EXPECT_EQ(state_change_1->human_readable_diff, "Receive 7975.46196 USDC");
  EXPECT_EQ(state_change_1->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kErc20Transfer);
  ASSERT_TRUE(state_change_1->raw_info->data->is_erc20_transfer_data());
  const auto& state_change_1_raw_info =
      state_change_1->raw_info->data->get_erc20_transfer_data();
  EXPECT_EQ(state_change_1_raw_info->amount->before, "0");
  EXPECT_EQ(state_change_1_raw_info->amount->after, "7975461958");
  ASSERT_TRUE(state_change_1_raw_info->counterparty);
  EXPECT_EQ(state_change_1_raw_info->counterparty->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_1_raw_info->counterparty->address,
            "0x397ff1542f962076d0bfe58ea045ffa2d347aca0");
  ASSERT_TRUE(state_change_1_raw_info->asset);
  EXPECT_EQ(state_change_1_raw_info->asset->address,
            "0xa0b86991c6218b36c1d19d4a2e9eb0ce3606eb48");
  EXPECT_EQ(state_change_1_raw_info->asset->symbol, "USDC");
  EXPECT_EQ(state_change_1_raw_info->asset->name, "USDCoin");
  EXPECT_EQ(state_change_1_raw_info->asset->decimals, 6);
  EXPECT_TRUE(state_change_1_raw_info->asset->verified);
  ASSERT_EQ(state_change_1_raw_info->asset->lists.size(), 6u);
  EXPECT_EQ(state_change_1_raw_info->asset->lists.at(0), "COINGECKO");
  EXPECT_EQ(state_change_1_raw_info->asset->lists.at(1), "ZERION");
  EXPECT_EQ(state_change_1_raw_info->asset->lists.at(2), "ONE_INCH");
  EXPECT_EQ(state_change_1_raw_info->asset->lists.at(3), "UNISWAP");
  EXPECT_EQ(state_change_1_raw_info->asset->lists.at(4), "MY_CRYPTO_API");
  EXPECT_EQ(state_change_1_raw_info->asset->lists.at(5), "KLEROS_TOKENS");
  EXPECT_EQ(state_change_1_raw_info->asset->image_url, "https://usdc.png");
  EXPECT_EQ(state_change_1_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);
  EXPECT_EQ(state_change_1_raw_info->asset->price->last_updated_at,
            "1689251215");
  EXPECT_EQ(state_change_1_raw_info->asset->price->dollar_value_per_token,
            "0.999694");

  const auto& state_change_2 =
      simulation_response->expected_state_changes.at(2);
  EXPECT_EQ(state_change_2->human_readable_diff,
            "Approve to transfer any amount of your SYN");
  EXPECT_EQ(state_change_2->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kErc20Approval);
  ASSERT_TRUE(state_change_2->raw_info->data->is_erc20_approval_data());
  const auto& state_change_2_raw_info =
      state_change_2->raw_info->data->get_erc20_approval_data();
  EXPECT_EQ(state_change_2_raw_info->amount->before,
            "115792089237316195423570985008687907853269984665640564039457584007"
            "913129639935");
  EXPECT_EQ(state_change_2_raw_info->amount->after,
            "115792089237316195423570985008687907853269984665640564033933833222"
            "991078811583");
  ASSERT_TRUE(state_change_2_raw_info->spender);
  EXPECT_EQ(state_change_2_raw_info->spender->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_2_raw_info->spender->address,
            "0xd9e1ce17f2641f24ae83637ab66a2cca9c378b9f");
  ASSERT_TRUE(state_change_2_raw_info->owner);
  EXPECT_EQ(state_change_2_raw_info->owner->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_2_raw_info->owner->address,
            "0x06924592cdf28acd3c1d23c37875c6c6a667bdf7");
  ASSERT_TRUE(state_change_2_raw_info->asset);
  EXPECT_EQ(state_change_2_raw_info->asset->address,
            "0x0f2d719407fdbeff09d87557abb7232601fd9f29");
  EXPECT_EQ(state_change_2_raw_info->asset->symbol, "SYN");
  EXPECT_EQ(state_change_2_raw_info->asset->name, "Synapse");
  EXPECT_EQ(state_change_2_raw_info->asset->decimals, 18);
  EXPECT_TRUE(state_change_2_raw_info->asset->verified);
  ASSERT_EQ(state_change_2_raw_info->asset->lists.size(), 3u);
  EXPECT_EQ(state_change_2_raw_info->asset->lists.at(0), "COINGECKO");
  EXPECT_EQ(state_change_2_raw_info->asset->lists.at(1), "ONE_INCH");
  EXPECT_EQ(state_change_2_raw_info->asset->lists.at(2), "UNISWAP");
  EXPECT_EQ(state_change_2_raw_info->asset->image_url, "https://syn.png");
  EXPECT_EQ(state_change_2_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);
  EXPECT_EQ(state_change_2_raw_info->asset->price->last_updated_at,
            "1689251218");
  EXPECT_EQ(state_change_2_raw_info->asset->price->dollar_value_per_token,
            "0.645586");
}

// Example from https://docs.blowfish.xyz/reference/scan-transactions-evm
TEST(SimulationResponseParserUnitTest, ParseEvmErc20ApprovalMalicious) {
  std::string json(R"(
    {
      "requestId":"e8cd35ce-f743-4ef2-8e94-f26857744db7",
      "action":"BLOCK",
      "warnings":[
        {
          "kind":"KNOWN_MALICIOUS",
          "message":"We believe this transaction is malicious and unsafe to sign. Approving may lead to loss of funds.",
          "severity":"CRITICAL"
        }
      ],
      "simulationResults":{
        "aggregated":{
          "error":null,
          "expectedStateChanges":{
            "0xd8da6bf26964af9d7eed9e03e53415d37aa96045":[
              {
                "humanReadableDiff":"Approve to transfer up to 1000 USDT",
                "rawInfo":{
                  "kind":"ERC20_APPROVAL",
                  "data":{
                    "amount":{
                      "after":"1000000000",
                      "before":"0"
                    },
                    "asset":{
                      "address":"0xdac17f958d2ee523a2206206994597c13d831ec7",
                      "name":"Tether USD",
                      "decimals":"6",
                      "lists":[
                        "COINGECKO",
                        "ZERION",
                        "ONE_INCH",
                        "UNISWAP",
                        "MY_CRYPTO_API",
                        "KLEROS_TOKENS"
                      ],
                      "symbol":"USDT",
                      "verified":true,
                      "imageUrl":"https://usdt.png",
                      "price":{
                        "source":"Coingecko",
                        "updatedAt":"1679331222",
                        "dollarValuePerToken":"0.99"
                      }
                    },
                    "owner":{
                      "address":"0xd8da6bf26964af9d7eed9e03e53415d37aa96045",
                      "kind":"ACCOUNT"
                    },
                    "spender":{
                      "address":"0x1d5071048370df50839c8879cdf5144ace4b3b3b",
                      "kind":"ACCOUNT"
                    }
                  }
                }
              }
            ]
          },
          "userAccount":"0xd8da6bf26964af9d7eed9e03e53415d37aa96045"
        }
      }
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(
      ParseJson(json), "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action,
            mojom::BlowfishSuggestedAction::kBlock);
  ASSERT_EQ(simulation_response->warnings.size(), 1u);
  EXPECT_EQ(simulation_response->warnings.at(0)->kind,
            mojom::BlowfishWarningKind::kKnownMalicious);
  EXPECT_EQ(simulation_response->warnings.at(0)->message,
            "We believe this transaction is malicious and unsafe to sign. "
            "Approving may lead to loss of funds.");
  EXPECT_EQ(simulation_response->warnings.at(0)->severity,
            mojom::BlowfishWarningSeverity::kCritical);
  EXPECT_FALSE(simulation_response->error);
  ASSERT_EQ(simulation_response->expected_state_changes.size(), 1u);

  const auto& state_change = simulation_response->expected_state_changes.at(0);
  EXPECT_EQ(state_change->human_readable_diff,
            "Approve to transfer up to 1000 USDT");
  EXPECT_EQ(state_change->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kErc20Approval);
  ASSERT_TRUE(state_change->raw_info->data->is_erc20_approval_data());
  const auto& state_change_raw_info =
      state_change->raw_info->data->get_erc20_approval_data();
  EXPECT_EQ(state_change_raw_info->amount->before, "0");
  EXPECT_EQ(state_change_raw_info->amount->after, "1000000000");
  ASSERT_TRUE(state_change_raw_info->spender);
  EXPECT_EQ(state_change_raw_info->spender->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->spender->address,
            "0x1d5071048370df50839c8879cdf5144ace4b3b3b");
  ASSERT_TRUE(state_change_raw_info->owner);
  EXPECT_EQ(state_change_raw_info->owner->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->owner->address,
            "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
  ASSERT_TRUE(state_change_raw_info->asset);
  EXPECT_EQ(state_change_raw_info->asset->address,
            "0xdac17f958d2ee523a2206206994597c13d831ec7");
  EXPECT_EQ(state_change_raw_info->asset->symbol, "USDT");
  EXPECT_EQ(state_change_raw_info->asset->name, "Tether USD");
  EXPECT_EQ(state_change_raw_info->asset->decimals, 6);
  EXPECT_TRUE(state_change_raw_info->asset->verified);
  ASSERT_EQ(state_change_raw_info->asset->lists.size(), 6u);
  EXPECT_EQ(state_change_raw_info->asset->lists.at(0), "COINGECKO");
  EXPECT_EQ(state_change_raw_info->asset->lists.at(1), "ZERION");
  EXPECT_EQ(state_change_raw_info->asset->lists.at(2), "ONE_INCH");
  EXPECT_EQ(state_change_raw_info->asset->lists.at(3), "UNISWAP");
  EXPECT_EQ(state_change_raw_info->asset->lists.at(4), "MY_CRYPTO_API");
  EXPECT_EQ(state_change_raw_info->asset->lists.at(5), "KLEROS_TOKENS");
  EXPECT_EQ(state_change_raw_info->asset->image_url, "https://usdt.png");
  EXPECT_EQ(state_change_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);
  EXPECT_EQ(state_change_raw_info->asset->price->last_updated_at, "1679331222");
  EXPECT_EQ(state_change_raw_info->asset->price->dollar_value_per_token,
            "0.99");
}

// Example from https://docs.blowfish.xyz/reference/scan-transactions-evm
TEST(SimulationResponseParserUnitTest, ParseEvmBuyErc721NftWithEth) {
  std::string json(R"(
    {
      "requestId":"e8cd35ce-f743-4ef2-8e94-f26857744db7",
      "action":"NONE",
      "warnings":[],
      "simulationResults":{
        "aggregated":{
          "expectedStateChanges":{
            "0xd8da6bf26964af9d7eed9e03e53415d37aa96045":[
              {
                "humanReadableDiff":"Receive PudgyPenguins #7238",
                "rawInfo":{
                  "kind":"ERC721_TRANSFER",
                  "data":{
                    "amount":{
                      "after":"1",
                      "before":"0"
                    },
                    "metadata":{
                      "rawImageUrl":"https://pudgy-penguins.png"
                    },
                    "tokenId":"7238",
                    "asset":{
                      "address":"0xbd3531da5cf5857e7cfaa92426877b022e612cf8",
                      "name":"PudgyPenguins",
                      "collection":"PudgyPenguins",
                      "symbol":"PPG",
                      "price":{
                        "source":"Simplehash",
                        "updatedAt":"1679331222",
                        "dollarValuePerToken":"594.99"
                      }
                    },
                    "counterparty":{
                      "kind":"ACCOUNT",
                      "address":"0x06924592cdf28acd3c1d23c37875c6c6a667bdf7"
                    }
                  }
                }
              },
              {
                "humanReadableDiff":"Send 3.181 ETH",
                "rawInfo":{
                  "kind":"NATIVE_ASSET_TRANSFER",
                  "data":{
                    "amount":{
                      "after":"998426264937289938488",
                      "before":"1001607264937289938488"
                    },
                    "contract":{
                      "address":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                      "kind":"ACCOUNT"
                    },
                    "asset":{
                      "address":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                      "symbol":"ETH",
                      "name":"Ether",
                      "decimals":"18",
                      "verified":true,
                      "imageUrl":"https//eth.png",
                      "price":{
                        "source":"Coingecko",
                        "updatedAt":"1681958792",
                        "dollarValuePerToken":"1945.92"
                      }
                    },
                    "counterparty":{
                      "kind":"ACCOUNT",
                      "address":"0x06924592cdf28acd3c1d23c37875c6c6a667bdf7"
                    }
                  }
                }
              }
            ]
          },
          "userAccount":"0xd8da6bf26964af9d7eed9e03e53415d37aa96045",
          "error":null
        }
      }
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(
      ParseJson(json), "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kNone);
  EXPECT_EQ(simulation_response->warnings.size(), 0u);
  EXPECT_FALSE(simulation_response->error);
  ASSERT_EQ(simulation_response->expected_state_changes.size(), 2u);

  const auto& state_change_0 =
      simulation_response->expected_state_changes.at(0);
  EXPECT_EQ(state_change_0->human_readable_diff, "Receive PudgyPenguins #7238");
  EXPECT_EQ(state_change_0->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kErc721Transfer);
  ASSERT_TRUE(state_change_0->raw_info->data->is_erc721_transfer_data());
  const auto& state_change_0_raw_info =
      state_change_0->raw_info->data->get_erc721_transfer_data();
  EXPECT_EQ(state_change_0_raw_info->amount->before, "0");
  EXPECT_EQ(state_change_0_raw_info->amount->after, "1");
  EXPECT_EQ(state_change_0_raw_info->metadata->raw_image_url,
            "https://pudgy-penguins.png");
  ASSERT_TRUE(state_change_0_raw_info->counterparty);
  EXPECT_EQ(state_change_0_raw_info->counterparty->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_0_raw_info->counterparty->address,
            "0x06924592cdf28acd3c1d23c37875c6c6a667bdf7");
  ASSERT_TRUE(state_change_0_raw_info->asset);
  EXPECT_EQ(state_change_0_raw_info->asset->address,
            "0xbd3531da5cf5857e7cfaa92426877b022e612cf8");
  EXPECT_EQ(state_change_0_raw_info->asset->symbol, "PPG");
  EXPECT_EQ(state_change_0_raw_info->asset->name, "PudgyPenguins");
  EXPECT_EQ(state_change_0_raw_info->asset->collection, "PudgyPenguins");
  EXPECT_EQ(state_change_0_raw_info->asset->token_id, "7238");
  EXPECT_EQ(state_change_0_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kSimplehash);
  EXPECT_EQ(state_change_0_raw_info->asset->price->last_updated_at,
            "1679331222");
  EXPECT_EQ(state_change_0_raw_info->asset->price->dollar_value_per_token,
            "594.99");

  const auto& state_change_1 =
      simulation_response->expected_state_changes.at(1);
  EXPECT_EQ(state_change_1->human_readable_diff, "Send 3.181 ETH");
  EXPECT_EQ(state_change_1->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kNativeAssetTransfer);
  ASSERT_TRUE(state_change_1->raw_info->data->is_native_asset_transfer_data());
  const auto& state_change_1_raw_info =
      state_change_1->raw_info->data->get_native_asset_transfer_data();
  EXPECT_EQ(state_change_1_raw_info->amount->before, "1001607264937289938488");
  EXPECT_EQ(state_change_1_raw_info->amount->after, "998426264937289938488");
  ASSERT_TRUE(state_change_1_raw_info->counterparty);
  EXPECT_EQ(state_change_1_raw_info->counterparty->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_1_raw_info->counterparty->address,
            "0x06924592cdf28acd3c1d23c37875c6c6a667bdf7");
  ASSERT_TRUE(state_change_1_raw_info->asset);
  EXPECT_EQ(state_change_1_raw_info->asset->address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  EXPECT_EQ(state_change_1_raw_info->asset->symbol, "ETH");
  EXPECT_EQ(state_change_1_raw_info->asset->name, "Ether");
  EXPECT_EQ(state_change_1_raw_info->asset->decimals, 18);
  EXPECT_TRUE(state_change_1_raw_info->asset->verified);
  ASSERT_EQ(state_change_1_raw_info->asset->lists.size(), 0u);
  EXPECT_EQ(state_change_1_raw_info->asset->image_url, "https//eth.png");
  EXPECT_EQ(state_change_1_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);
  EXPECT_EQ(state_change_1_raw_info->asset->price->last_updated_at,
            "1681958792");
  EXPECT_EQ(state_change_1_raw_info->asset->price->dollar_value_per_token,
            "1945.92");
}

// Example from https://docs.blowfish.xyz/reference/scan-transactions-evm
TEST(SimulationResponseParserUnitTest, ParseEvmErc721ApprovalForAll) {
  std::string json(R"(
    {
      "requestId":"e8cd35ce-f743-4ef2-8e94-f26857744db7",
      "action":"WARN",
      "warnings":[
        {
          "kind":"UNLIMITED_ALLOWANCE_TO_NFTS",
          "message":"You are allowing this website to withdraw funds from your account in the future.",
          "severity":"WARNING"
        }
      ],
      "simulationResults":{
        "aggregated":{
          "expectedStateChanges":{
            "0xd8da6bf26964af9d7eed9e03e53415d37aa96045":[
              {
                "humanReadableDiff":"Approve to transfer all your BoredApeYachtClub",
                "rawInfo":{
                  "kind":"ERC721_APPROVAL_FOR_ALL",
                  "data":{
                    "amount":{
                      "after":"115792089237316195423570985008687907853269984665640564039457584007913129639935",
                      "before":"0"
                    },
                    "owner":{
                      "address":"0x38191ca1307ebf67ca1a7caf5346dbd91d882ca6",
                      "kind":"ACCOUNT"
                    },
                    "spender":{
                      "address":"0x1e0049783f008a0085193e00003d00cd54003c71",
                      "kind":"ACCOUNT"
                    },
                    "asset":{
                      "address":"0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d",
                      "name":"BoredApeYachtClub",
                      "collection":"BoredApeYachtClub",
                      "symbol":"BAYC",
                      "price":{
                        "source":"Simplehash",
                        "updatedAt":"1679331222",
                        "dollarValuePerToken":"7865.43"
                      }
                    }
                  }
                }
              }
            ]
          },
          "userAccount":"0xd8da6bf26964af9d7eed9e03e53415d37aa96045",
          "error":null
        }
      }
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(
      ParseJson(json), "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kWarn);
  ASSERT_EQ(simulation_response->warnings.size(), 1u);
  EXPECT_EQ(simulation_response->warnings.at(0)->kind,
            mojom::BlowfishWarningKind::kUnlimitedAllowanceToNfts);
  EXPECT_EQ(simulation_response->warnings.at(0)->message,
            "You are allowing this website to withdraw funds from your account "
            "in the future.");
  EXPECT_EQ(simulation_response->warnings.at(0)->severity,
            mojom::BlowfishWarningSeverity::kWarning);
  EXPECT_FALSE(simulation_response->error);
  ASSERT_EQ(simulation_response->expected_state_changes.size(), 1u);

  const auto& state_change = simulation_response->expected_state_changes.at(0);
  EXPECT_EQ(state_change->human_readable_diff,
            "Approve to transfer all your BoredApeYachtClub");
  EXPECT_EQ(state_change->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kErc721ApprovalForAll);
  ASSERT_TRUE(state_change->raw_info->data->is_erc721_approval_for_all_data());
  const auto& state_change_raw_info =
      state_change->raw_info->data->get_erc721_approval_for_all_data();
  EXPECT_EQ(state_change_raw_info->amount->before, "0");
  EXPECT_EQ(state_change_raw_info->amount->after,
            "115792089237316195423570985008687907853269984665640564039457584007"
            "913129639935");
  ASSERT_TRUE(state_change_raw_info->owner);
  EXPECT_EQ(state_change_raw_info->owner->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->owner->address,
            "0x38191ca1307ebf67ca1a7caf5346dbd91d882ca6");
  ASSERT_TRUE(state_change_raw_info->spender);
  EXPECT_EQ(state_change_raw_info->spender->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->spender->address,
            "0x1e0049783f008a0085193e00003d00cd54003c71");
  ASSERT_TRUE(state_change_raw_info->asset);
  EXPECT_EQ(state_change_raw_info->asset->address,
            "0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d");
  EXPECT_EQ(state_change_raw_info->asset->symbol, "BAYC");
  EXPECT_EQ(state_change_raw_info->asset->name, "BoredApeYachtClub");
  EXPECT_EQ(state_change_raw_info->asset->collection, "BoredApeYachtClub");
  EXPECT_EQ(state_change_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kSimplehash);
  EXPECT_EQ(state_change_raw_info->asset->price->last_updated_at, "1679331222");
  EXPECT_EQ(state_change_raw_info->asset->price->dollar_value_per_token,
            "7865.43");
}

// Example from https://docs.blowfish.xyz/reference/scan-transactions-evm
TEST(SimulationResponseParserUnitTest, ParseEvmErc721Approval) {
  std::string json(R"(
    {
      "requestId":"e8cd35ce-f743-4ef2-8e94-f26857744db7",
      "action":"NONE",
      "warnings":[],
      "simulationResults":{
        "aggregated":{
          "expectedStateChanges":{
            "0xd8da6bf26964af9d7eed9e03e53415d37aa96045":[
              {
                "humanReadableDiff":"Approve to transfer BoredApeYachtClub",
                "rawInfo":{
                  "kind":"ERC721_APPROVAL",
                  "data":{
                    "amount":{
                      "after":"1",
                      "before":"0"
                    },
                    "metadata":{
                      "rawImageUrl":"https://bayc.png"
                    },
                    "owner":{
                      "address":"0xed2ab4948ba6a909a7751dec4f34f303eb8c7236",
                      "kind":"ACCOUNT"
                    },
                    "spender":{
                      "address":"0x1e0049783f008a0085193e00003d00cd54003c71",
                      "kind":"ACCOUNT"
                    },
                    "tokenId":"6603",
                    "asset":{
                      "address":"0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d",
                      "name":"BoredApeYachtClub",
                      "collection":"BoredApeYachtClub",
                      "symbol":"BAYC",
                      "price":{
                        "source":"Simplehash",
                        "updatedAt":"1679331222",
                        "dollarValuePerToken":"7865.43"
                      }
                    }
                  }
                }
              }
            ]
          },
          "userAccount":"0xd8da6bf26964af9d7eed9e03e53415d37aa96045",
          "error":null
        }
      }
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(
      ParseJson(json), "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kNone);
  EXPECT_EQ(simulation_response->warnings.size(), 0u);
  EXPECT_FALSE(simulation_response->error);
  ASSERT_EQ(simulation_response->expected_state_changes.size(), 1u);

  const auto& state_change = simulation_response->expected_state_changes.at(0);
  EXPECT_EQ(state_change->human_readable_diff,
            "Approve to transfer BoredApeYachtClub");
  EXPECT_EQ(state_change->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kErc721Approval);
  ASSERT_TRUE(state_change->raw_info->data->is_erc721_approval_data());
  const auto& state_change_raw_info =
      state_change->raw_info->data->get_erc721_approval_data();
  EXPECT_EQ(state_change_raw_info->amount->before, "0");
  EXPECT_EQ(state_change_raw_info->amount->after, "1");
  EXPECT_EQ(state_change_raw_info->metadata->raw_image_url, "https://bayc.png");
  ASSERT_TRUE(state_change_raw_info->owner);
  EXPECT_EQ(state_change_raw_info->owner->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->owner->address,
            "0xed2ab4948ba6a909a7751dec4f34f303eb8c7236");
  ASSERT_TRUE(state_change_raw_info->spender);
  EXPECT_EQ(state_change_raw_info->spender->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->spender->address,
            "0x1e0049783f008a0085193e00003d00cd54003c71");
  ASSERT_TRUE(state_change_raw_info->asset);
  EXPECT_EQ(state_change_raw_info->asset->address,
            "0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d");
  EXPECT_EQ(state_change_raw_info->asset->symbol, "BAYC");
  EXPECT_EQ(state_change_raw_info->asset->name, "BoredApeYachtClub");
  EXPECT_EQ(state_change_raw_info->asset->collection, "BoredApeYachtClub");
  EXPECT_EQ(state_change_raw_info->asset->token_id, "6603");
  EXPECT_EQ(state_change_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kSimplehash);
  EXPECT_EQ(state_change_raw_info->asset->price->last_updated_at, "1679331222");
  EXPECT_EQ(state_change_raw_info->asset->price->dollar_value_per_token,
            "7865.43");
}

// Example from https://docs.blowfish.xyz/reference/scan-transactions-evm
TEST(SimulationResponseParserUnitTest, ParseEvmBuyErc1155TokenWithEth) {
  std::string json(R"(
    {
      "requestId":"e8cd35ce-f743-4ef2-8e94-f26857744db7",
      "action":"NONE",
      "simulationResults":{
        "aggregated":{
          "error":null,
          "userAccount":"0xd8da6bf26964af9d7eed9e03e53415d37aa96045",
          "expectedStateChanges":{
            "0xd8da6bf26964af9d7eed9e03e53415d37aa96045":[
              {
                "humanReadableDiff":"Send 0.033 ETH",
                "rawInfo":{
                  "kind":"NATIVE_ASSET_TRANSFER",
                  "data":{
                    "amount":{
                      "after":"71057321770366572",
                      "before":"104057321770366572"
                    },
                    "contract":{
                      "address":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                      "kind":"ACCOUNT"
                    },
                    "asset":{
                      "address":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                      "symbol":"ETH",
                      "name":"Ether",
                      "decimals":"18",
                      "verified":true,
                      "imageUrl":"https://eth.png",
                      "price":{
                        "source":"Coingecko",
                        "updatedAt":"1681958792",
                        "dollarValuePerToken":"1945.92"
                      }
                    },
                    "counterparty":{
                      "kind":"ACCOUNT",
                      "address":"0x06924592cdf28acd3c1d23c37875c6c6a667bdf7"
                    }
                  }
                }
              },
              {
                "humanReadableDiff":"Receive Corgi",
                "rawInfo":{
                  "kind":"ERC1155_TRANSFER",
                  "data":{
                    "amount":{
                      "after":"1",
                      "before":"0"
                    },
                    "metadata":{
                      "rawImageUrl":"https://corgi.png"
                    },
                    "tokenId":"13014975",
                    "counterparty":{
                      "kind":"ACCOUNT",
                      "address":"0x06924592cdf28acd3c1d23c37875c6c6a667bdf7"
                    },
                    "asset":{
                      "address":"0x51e613727fdd2e0b91b51c3e5427e9440a7957e4",
                      "name":"Corgi",
                      "price":{
                        "source":"Simplehash",
                        "updatedAt":"1679331222",
                        "dollarValuePerToken":"232.43"
                      }
                    }
                  }
                }
              }
            ]
          }
        }
      },
      "warnings":[]
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(
      ParseJson(json), "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kNone);
  EXPECT_EQ(simulation_response->warnings.size(), 0u);
  EXPECT_FALSE(simulation_response->error);
  ASSERT_EQ(simulation_response->expected_state_changes.size(), 2u);

  const auto& state_change_0 =
      simulation_response->expected_state_changes.at(0);
  EXPECT_EQ(state_change_0->human_readable_diff, "Send 0.033 ETH");
  EXPECT_EQ(state_change_0->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kNativeAssetTransfer);
  ASSERT_TRUE(state_change_0->raw_info->data->is_native_asset_transfer_data());
  const auto& state_change_0_raw_info =
      state_change_0->raw_info->data->get_native_asset_transfer_data();
  EXPECT_EQ(state_change_0_raw_info->amount->before, "104057321770366572");
  EXPECT_EQ(state_change_0_raw_info->amount->after, "71057321770366572");
  ASSERT_TRUE(state_change_0_raw_info->counterparty);
  EXPECT_EQ(state_change_0_raw_info->counterparty->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_0_raw_info->counterparty->address,
            "0x06924592cdf28acd3c1d23c37875c6c6a667bdf7");
  ASSERT_TRUE(state_change_0_raw_info->asset);
  EXPECT_EQ(state_change_0_raw_info->asset->address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  EXPECT_EQ(state_change_0_raw_info->asset->symbol, "ETH");
  EXPECT_EQ(state_change_0_raw_info->asset->name, "Ether");
  EXPECT_EQ(state_change_0_raw_info->asset->decimals, 18);
  EXPECT_TRUE(state_change_0_raw_info->asset->verified);
  ASSERT_EQ(state_change_0_raw_info->asset->lists.size(), 0u);
  EXPECT_EQ(state_change_0_raw_info->asset->image_url, "https://eth.png");
  EXPECT_EQ(state_change_0_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);
  EXPECT_EQ(state_change_0_raw_info->asset->price->last_updated_at,
            "1681958792");
  EXPECT_EQ(state_change_0_raw_info->asset->price->dollar_value_per_token,
            "1945.92");

  const auto& state_change_1 =
      simulation_response->expected_state_changes.at(1);
  EXPECT_EQ(state_change_1->human_readable_diff, "Receive Corgi");
  EXPECT_EQ(state_change_1->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kErc1155Transfer);
  ASSERT_TRUE(state_change_1->raw_info->data->is_erc1155_transfer_data());
  const auto& state_change_1_raw_info =
      state_change_1->raw_info->data->get_erc1155_transfer_data();
  EXPECT_EQ(state_change_1_raw_info->amount->before, "0");
  EXPECT_EQ(state_change_1_raw_info->amount->after, "1");
  EXPECT_EQ(state_change_1_raw_info->metadata->raw_image_url,
            "https://corgi.png");
  ASSERT_TRUE(state_change_1_raw_info->counterparty);
  EXPECT_EQ(state_change_1_raw_info->counterparty->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_1_raw_info->counterparty->address,
            "0x06924592cdf28acd3c1d23c37875c6c6a667bdf7");
  ASSERT_TRUE(state_change_1_raw_info->asset);
  EXPECT_EQ(state_change_1_raw_info->asset->address,
            "0x51e613727fdd2e0b91b51c3e5427e9440a7957e4");
  EXPECT_EQ(state_change_1_raw_info->asset->name, "Corgi");
  EXPECT_EQ(state_change_1_raw_info->asset->token_id, "13014975");
  EXPECT_EQ(state_change_1_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kSimplehash);
  EXPECT_EQ(state_change_1_raw_info->asset->price->last_updated_at,
            "1679331222");
  EXPECT_EQ(state_change_1_raw_info->asset->price->dollar_value_per_token,
            "232.43");
}

// Example from https://docs.blowfish.xyz/reference/scan-transactions-evm
TEST(SimulationResponseParserUnitTest, ParseEvmErc1155ApprovalForAll) {
  std::string json(R"(
    {
      "requestId":"e8cd35ce-f743-4ef2-8e94-f26857744db7",
      "action":"WARN",
      "warnings":[
        {
          "kind":"UNLIMITED_ALLOWANCE_TO_NFTS",
          "message":"You are allowing this website to withdraw funds from your account in the future.",
          "severity":"WARNING"
        }
      ],
      "simulationResults":{
        "aggregated":{
          "error":null,
          "userAccount":"0xd8da6bf26964af9d7eed9e03e53415d37aa96045",
          "expectedStateChanges":{
            "0xd8da6bf26964af9d7eed9e03e53415d37aa96045":[
              {
                "humanReadableDiff":"Approve to transfer all your Sandbox's ASSETs",
                "rawInfo":{
                  "kind":"ERC1155_APPROVAL_FOR_ALL",
                  "data":{
                    "amount":{
                      "after":"115792089237316195423570985008687907853269984665640564039457584007913129639935",
                      "before":"0"
                    },
                    "owner":{
                      "address":"0xed2ab4948ba6a909a7751dec4f34f303eb8c7236",
                      "kind":"ACCOUNT"
                    },
                    "spender":{
                      "address":"0x00000000006c3852cbef3e08e8df289169ede581",
                      "kind":"ACCOUNT"
                    },
                    "asset":{
                      "address":"0xa342f5d851e866e18ff98f351f2c6637f4478db5",
                      "name":%s,
                      "price":{
                        "source":"Simplehash",
                        "updatedAt":"1679331222",
                        "dollarValuePerToken":"232.43"
                      }
                    }
                  }
                }
              }
            ]
          }
        }
      }
    }
  )");

  auto json_with_token_name =
      base::StringPrintf(json.c_str(), "\"Sandbox ASSET\"");

  auto simulation_response = evm::ParseSimulationResponse(
      ParseJson(json_with_token_name),
      "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kWarn);
  ASSERT_EQ(simulation_response->warnings.size(), 1u);
  EXPECT_EQ(simulation_response->warnings.at(0)->kind,
            mojom::BlowfishWarningKind::kUnlimitedAllowanceToNfts);
  EXPECT_EQ(simulation_response->warnings.at(0)->message,
            "You are allowing this website to withdraw funds from your account "
            "in the future.");
  EXPECT_EQ(simulation_response->warnings.at(0)->severity,
            mojom::BlowfishWarningSeverity::kWarning);
  EXPECT_FALSE(simulation_response->error);
  ASSERT_EQ(simulation_response->expected_state_changes.size(), 1u);

  const auto& state_change = simulation_response->expected_state_changes.at(0);
  EXPECT_EQ(state_change->human_readable_diff,
            "Approve to transfer all your Sandbox's ASSETs");
  EXPECT_EQ(state_change->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kErc1155ApprovalForAll);
  ASSERT_TRUE(state_change->raw_info->data->is_erc1155_approval_for_all_data());
  const auto& state_change_raw_info =
      state_change->raw_info->data->get_erc1155_approval_for_all_data();
  EXPECT_EQ(state_change_raw_info->amount->before, "0");
  EXPECT_EQ(state_change_raw_info->amount->after,
            "115792089237316195423570985008687907853269984665640564039457584007"
            "913129639935");
  ASSERT_TRUE(state_change_raw_info->owner);
  EXPECT_EQ(state_change_raw_info->owner->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->owner->address,
            "0xed2ab4948ba6a909a7751dec4f34f303eb8c7236");
  ASSERT_TRUE(state_change_raw_info->spender);
  EXPECT_EQ(state_change_raw_info->spender->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->spender->address,
            "0x00000000006c3852cbef3e08e8df289169ede581");
  ASSERT_TRUE(state_change_raw_info->asset);
  EXPECT_EQ(state_change_raw_info->asset->address,
            "0xa342f5d851e866e18ff98f351f2c6637f4478db5");
  EXPECT_EQ(state_change_raw_info->asset->name, "Sandbox ASSET");
  EXPECT_EQ(state_change_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kSimplehash);
  EXPECT_EQ(state_change_raw_info->asset->price->last_updated_at, "1679331222");
  EXPECT_EQ(state_change_raw_info->asset->price->dollar_value_per_token,
            "232.43");

  json_with_token_name = base::StringPrintf(json.c_str(), "null");
  EXPECT_TRUE(evm::ParseSimulationResponse(
      ParseJson(json_with_token_name),
      "0xd8da6bf26964af9d7eed9e03e53415d37aa96045"));

  json_with_token_name = base::StringPrintf(json.c_str(), "[]");
  EXPECT_FALSE(evm::ParseSimulationResponse(
      ParseJson(json_with_token_name),
      "0xd8da6bf26964af9d7eed9e03e53415d37aa96045"));

  json_with_token_name = base::StringPrintf(json.c_str(), "{}");
  EXPECT_FALSE(evm::ParseSimulationResponse(
      ParseJson(json_with_token_name),
      "0xd8da6bf26964af9d7eed9e03e53415d37aa96045"));

  json_with_token_name = base::StringPrintf(json.c_str(), "false");
  EXPECT_FALSE(evm::ParseSimulationResponse(
      ParseJson(json_with_token_name),
      "0xd8da6bf26964af9d7eed9e03e53415d37aa96045"));
}

TEST(SimulationResponseParserUnitTest, ParseEvmSimulationErrorResponse) {
  std::string json(R"(
    {
      "error": "No transactions to simulate"
    }
  )");

  auto error_response = ParseSimulationErrorResponse(ParseJson(json));
  ASSERT_TRUE(error_response);
  EXPECT_EQ(*error_response, "No transactions to simulate");

  json = R"(
    {
      "foo": "bar"
    }
  )";
  error_response = ParseSimulationErrorResponse(ParseJson(json));
  EXPECT_FALSE(error_response);
}

TEST(SimulationResponseParserUnitTest, ParseEvmInvalidDecimals) {
  std::string json(R"(
    {
      "requestId":"e8cd35ce-f743-4ef2-8e94-f26857744db7",
      "action":"NONE",
      "warnings":[],
      "simulationResults":{
        "aggregated":{
          "error":null,
          "userAccount":"0xd8da6bf26964af9d7eed9e03e53415d37aa96045",
          "expectedStateChanges":{
            "0xd8da6bf26964af9d7eed9e03e53415d37aa96045":[
              {
                "humanReadableDiff":"Send 3.181 ETH",
                "rawInfo":{
                  "kind":"NATIVE_ASSET_TRANSFER",
                  "data":{
                    "amount":{
                      "after":"998426264937289938488",
                      "before":"1001607264937289938488"
                    },
                    "asset":{
                      "address":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                      "symbol":"ETH",
                      "name":"Ether",
                      "decimals":"boo",
                      "verified":true,
                      "imageUrl":"https//eth.png",
                      "price":null
                    },
                    "counterparty":{
                      "kind":"ACCOUNT",
                      "address":"0x06924592cdf28acd3c1d23c37875c6c6a667bdf7"
                    }
                  }
                }
              }
            ]
          }
        }
      }
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(
      ParseJson(json), "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
  EXPECT_FALSE(simulation_response);
}

TEST(SimulationResponseParserUnitTest, ParseEvmUnknownError) {
  std::string json(R"(
    {
      "requestId":"e8cd35ce-f743-4ef2-8e94-f26857744db7",
      "action":"NONE",
      "warnings":[],
      "simulationResults":{
        "aggregated": {
          "error": {
            "humanReadableError": "Unable to simulate transaction",
            "kind": "UNKNOWN_ERROR"
          },
          "expectedStateChanges": {
            "0xd8da6bf26964af9d7eed9e03e53415d37aa96045": []
          },
        }
      }
    }
  )");
  auto simulation_response = evm::ParseSimulationResponse(
      ParseJson(json), "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kNone);
  EXPECT_EQ(simulation_response->warnings.size(), 0ULL);
  ASSERT_TRUE(simulation_response->error);
  EXPECT_EQ(simulation_response->error->kind,
            mojom::BlowfishEVMErrorKind::kUnknownError);
  EXPECT_EQ(simulation_response->error->human_readable_error,
            "Unable to simulate transaction");
  EXPECT_EQ(simulation_response->expected_state_changes.size(), 0ULL);
}

TEST(SimulationResponseParserUnitTest, ParseEvmUnknownWarnings) {
  std::string json(R"(
    {
      "requestId":"e8cd35ce-f743-4ef2-8e94-f26857744db7",
      "action":"BLOCK",
      "warnings": [
        {
          "severity": "CRITICAL",
          "kind": "THIS_IS_AN_UNKNOWN_CRITICAL_ERROR",
          "message": "There's something wrong with this transaction, but we don't know what it is."
        },
        {
          "severity": "WARNING",
          "kind": "SUSPECTED_MALICIOUS",
          "message": "We suspect this transaction is malicious. Approving may lead to loss of funds."
        }
      ],
      "simulationResults":{
        "aggregated":{
          "error":null,
          "userAccount":"0xd8da6bf26964af9d7eed9e03e53415d37aa96045",
          "expectedStateChanges":{
            "0xd8da6bf26964af9d7eed9e03e53415d37aa96045": []
          }
        }
      }
    }
  )");
  auto simulation_response = evm::ParseSimulationResponse(
      ParseJson(json), "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action,
            mojom::BlowfishSuggestedAction::kBlock);
  ASSERT_EQ(simulation_response->warnings.size(), 2ULL);

  EXPECT_EQ(simulation_response->warnings.at(0)->severity,
            mojom::BlowfishWarningSeverity::kCritical);
  EXPECT_EQ(simulation_response->warnings.at(0)->kind,
            mojom::BlowfishWarningKind::kUnknown);
  EXPECT_EQ(simulation_response->warnings.at(0)->message,
            "There's something wrong with this transaction, but we don't know "
            "what it is.");

  EXPECT_EQ(simulation_response->warnings.at(1)->severity,
            mojom::BlowfishWarningSeverity::kWarning);
  EXPECT_EQ(simulation_response->warnings.at(1)->kind,
            mojom::BlowfishWarningKind::kSuspectedMalicious);
  EXPECT_EQ(simulation_response->warnings.at(1)->message,
            "We suspect this transaction is malicious. Approving may lead to "
            "loss of funds.");

  EXPECT_FALSE(simulation_response->error);
  EXPECT_EQ(simulation_response->expected_state_changes.size(), 0ULL);
}

TEST(SimulationResponseParserUnitTest, ParseEvmNullableFields) {
  std::string json_fmt(R"(
    {
      "requestId":"e8cd35ce-f743-4ef2-8e94-f26857744db7",
      "action":"NONE",
      "warnings":[],
      "simulationResults":{
        "aggregated":{
          "error":null,
          "userAccount":"0xd8da6bf26964af9d7eed9e03e53415d37aa96045",
          "expectedStateChanges":{
            "0xd8da6bf26964af9d7eed9e03e53415d37aa96045":[
              {
                "humanReadableDiff":"Send 3.181 ETH",
                "rawInfo":{
                  "kind":"NATIVE_ASSET_TRANSFER",
                  "data":{
                    "amount":{
                      "after":"998426264937289938488",
                      "before":"1001607264937289938488"
                    },
                    "asset":{
                      "address":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                      "symbol":%s,
                      "name":%s,
                      "decimals":"18",
                      "verified":true,
                      "imageUrl":%s,
                      "price":%s
                    },
                    "counterparty":{
                      "kind":"ACCOUNT",
                      "address":"0x06924592cdf28acd3c1d23c37875c6c6a667bdf7"
                    }
                  }
                }
              }
            ]
          }
        }
      }
    }
  )");

  {
    auto json =
        base::StringPrintf(json_fmt.c_str(), "null", "null", "null", "null");
    auto simulation_response = evm::ParseSimulationResponse(
        ParseJson(json), "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");

    // OK: null values are allowed for the following fields:
    //     - asset->imageUrl
    //     - asset->price
    //     - asset->name
    //     - asset->symbol
    ASSERT_TRUE(simulation_response);

    EXPECT_EQ(simulation_response->action,
              mojom::BlowfishSuggestedAction::kNone);
    EXPECT_EQ(simulation_response->warnings.size(), 0ULL);
    EXPECT_FALSE(simulation_response->error);
    ASSERT_EQ(simulation_response->expected_state_changes.size(), 1ULL);

    const auto& state_change =
        simulation_response->expected_state_changes.at(0);
    EXPECT_EQ(state_change->human_readable_diff, "Send 3.181 ETH");
    EXPECT_EQ(state_change->raw_info->kind,
              mojom::BlowfishEVMRawInfoKind::kNativeAssetTransfer);
    ASSERT_TRUE(state_change->raw_info->data->is_native_asset_transfer_data());
    const auto& state_change_raw_info =
        state_change->raw_info->data->get_native_asset_transfer_data();
    EXPECT_EQ(state_change_raw_info->amount->before, "1001607264937289938488");
    EXPECT_EQ(state_change_raw_info->amount->after, "998426264937289938488");
    ASSERT_TRUE(state_change_raw_info->counterparty);
    EXPECT_EQ(state_change_raw_info->counterparty->kind,
              mojom::BlowfishEVMAddressKind::kAccount);
    EXPECT_EQ(state_change_raw_info->counterparty->address,
              "0x06924592cdf28acd3c1d23c37875c6c6a667bdf7");
    ASSERT_TRUE(state_change_raw_info->asset);
    EXPECT_EQ(state_change_raw_info->asset->address,
              "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
    EXPECT_EQ(state_change_raw_info->asset->symbol, "");
    EXPECT_EQ(state_change_raw_info->asset->name, "");
    EXPECT_EQ(state_change_raw_info->asset->decimals, 18);
    EXPECT_TRUE(state_change_raw_info->asset->verified);
    ASSERT_EQ(state_change_raw_info->asset->lists.size(), 0u);
    EXPECT_EQ(state_change_raw_info->asset->image_url, "");
    EXPECT_FALSE(state_change_raw_info->asset->price);
  }

  {
    auto json =
        base::StringPrintf(json_fmt.c_str(), "null", "null", "true", "null");
    // OK: invalid values for nullable field asset->imageUrl are treated as
    // null.
    auto simulation_response = evm::ParseSimulationResponse(
        ParseJson(json), "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
    ASSERT_TRUE(simulation_response);
    const auto& state_change =
        simulation_response->expected_state_changes.at(0);
    const auto& state_change_raw_info =
        state_change->raw_info->data->get_native_asset_transfer_data();
    EXPECT_EQ(state_change_raw_info->asset->image_url, "");
  }

  {
    auto json =
        base::StringPrintf(json_fmt.c_str(), "null", "null", "null", "true");
    // OK: invalid values for nullable field asset->price are treated as
    // null.
    auto simulation_response = evm::ParseSimulationResponse(
        ParseJson(json), "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
    ASSERT_TRUE(simulation_response);
    const auto& state_change =
        simulation_response->expected_state_changes.at(0);
    const auto& state_change_raw_info =
        state_change->raw_info->data->get_native_asset_transfer_data();
    EXPECT_FALSE(state_change_raw_info->asset->price);
  }

  {
    auto json = base::StringPrintf(json_fmt.c_str(), "null", "null", "null",
                                   "{\"foo\": 1}");
    // OK: invalid dict for nullable field asset->price is treated as null.
    auto simulation_response = evm::ParseSimulationResponse(
        ParseJson(json), "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
    ASSERT_TRUE(simulation_response);
    const auto& state_change =
        simulation_response->expected_state_changes.at(0);
    const auto& state_change_raw_info =
        state_change->raw_info->data->get_native_asset_transfer_data();
    EXPECT_FALSE(state_change_raw_info->asset->price);
  }
}

TEST(SimulationResponseParserUnitTest, ParseEvmInvalidRawInfoData) {
  std::string json(R"(
    {
      "requestId":"e8cd35ce-f743-4ef2-8e94-f26857744db7",
      "action":"NONE",
      "warnings":[],
      "simulationResults":{
        "aggregated":{
          "error": null,
          "userAccount":"0xd8da6bf26964af9d7eed9e03e53415d37aa96045",
          "expectedStateChanges":{
            "0xd8da6bf26964af9d7eed9e03e53415d37aa96045":[
              {
                "humanReadableDiff":"Send 3.181 ETH",
                "rawInfo":{
                  "kind":"NATIVE_ASSET_TRANSFER",
                  "data":null
                }
              }
            ]
          }
        }
      }
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(
      ParseJson(json), "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
  EXPECT_FALSE(simulation_response);
}

TEST(SimulationResponseParserUnitTest, ParseEvmInvalidError) {
  std::string json(R"(
    {
      "requestId":"e8cd35ce-f743-4ef2-8e94-f26857744db7",
      "action":"NONE",
      "warnings":[],
      "error":false,
      "simulationResults":{
        "aggregated":{
          "expectedStateChanges":{}
        }
      }
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(
      ParseJson(json), "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
  EXPECT_FALSE(simulation_response);
}

TEST(SimulationResponseParserUnitTest, ParseEvmResponseNotDict) {
  std::string json(R"([])");

  auto simulation_response = evm::ParseSimulationResponse(
      ParseJson(json), "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
  EXPECT_FALSE(simulation_response);
}

// Example from https://docs.blowfish.xyz/reference/scan-transactions-solana
TEST(SimulationResponseParserUnitTest, ParseSolanaStateChanges) {
  std::string json(R"(
    {
      "aggregated": {
        "action": "NONE",
        "warnings": [],
        "expectedStateChanges": {
          "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT": [
            {
              "humanReadableDiff": "Receive 0.05657 SOL",
              "suggestedColor": "CREDIT",
              "rawInfo": {
                "kind": "SOL_TRANSFER",
                "data": {
                  "asset": {
                    "symbol": "SOL",
                    "name": "Solana Native Token",
                    "decimals": "9",
                    "price": {
                      "source": "Coingecko",
                      "updatedAt": "1679331222",
                      "dollarValuePerToken": "100.92"
                    },
                    "imageUrl": "https://sol.png"
                  },
                  "diff": {
                    "sign": "PLUS",
                    "digits": "56573477"
                  }
                }
              }
            },
            {
              "humanReadableDiff": "Send 2 USDT",
              "suggestedColor": "DEBIT",
              "rawInfo": {
                "kind": "SPL_TRANSFER",
                "data": {
                  "asset": {
                    "symbol": "USDT",
                    "name": "USDT",
                    "mint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
                    "decimals": "6",
                    "supply": "1000000000",
                    "metaplexTokenStandard": "unknown",
                    "price": {
                      "source": "Coingecko",
                      "updatedAt": "1679331222",
                      "dollarValuePerToken": "0.99"
                    },
                    "imageUrl": "https://usdt.png"
                  },
                  "diff": {
                    "sign": "MINUS",
                    "digits": "2000000"
                  },
                  "counterparty": "5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X"
                }
              }
            },
            {
              "humanReadableDiff": "Approve to transfer Phantom QA NFT",
              "suggestedColor": "DEBIT",
              "rawInfo": {
                "kind": "SPL_APPROVAL",
                "data": {
                  "delegate": "CL38BiCb5fs3qGnKKSusaJdY24aFUUZ6vkvkujrmah83",
                  "asset": {
                    "symbol": "PHANTOMQA",
                    "name": "Phantom QA NFT",
                    "mint": "4ERKJVpwqS6Kcj115YSAjqU4WYV3YCDiYHPS72eQTY6p",
                    "decimals": "0",
                    "supply": "1",
                    "metaplexTokenStandard": "non_fungible",
                    "price": null
                  },
                  "diff": {
                    "sign": "PLUS",
                    "digits": "1525878906250000000"
                  }
                }
              }
            },
            {
              "humanReadableDiff": "Unapprove from transferring up to 0.00132 USDC",
              "suggestedColor": "CREDIT",
              "rawInfo": {
                "kind": "SPL_APPROVAL",
                "data": {
                  "delegate": "FCRBwXC5vrzHi2Vxgn3L2NB2KKFuRhiHBjioC47sSm2o",
                  "asset": {
                    "symbol": "USDC",
                    "name": "USD Coin",
                    "mint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                    "decimals": "6",
                    "supply": "5034964468128435",
                    "metaplexTokenStandard": "unknown",
                    "price": {
                      "source": "Coingecko",
                      "updatedAt": "1679331222",
                      "dollarValuePerToken": "1.01"
                    },
                    "imageUrl": "https://usdc.png"
                  },
                  "diff": {
                    "sign": "MINUS",
                    "digits": "1321"
                  }
                }
              }
            },
            {
              "humanReadableDiff": "Transfer control over your SOL staking account 2AG3be..p2vFQS",
              "suggestedColor": "DEBIT",
              "rawInfo": {
                "kind": "SOL_STAKE_AUTHORITY_CHANGE",
                "data": {
                  "stakeAccount": "2AG3beWwvyvEMLfwJcQS9DKMV62C3UWTTf8d7gp2vFQS",
                  "currentAuthorities": {
                    "staker": "J58MrVr9qJPzJJS8RPQUDfaFirN3PiVHXU48zr95FY48",
                    "withdrawer": "J58MrVr9qJPzJJS8RPQUDfaFirN3PiVHXU48zr95FY48"
                  },
                  "futureAuthorities": {
                    "staker": "EpochxXNkmM2akxBTuCEizW1oWyzgrPZ1CVZ3GpD7Egm",
                    "withdrawer": "EpochxXNkmM2akxBTuCEizW1oWyzgrPZ1CVZ3GpD7Egm"
                  },
                  "asset": {
                    "symbol": "SOL",
                    "name": "Solana Native Token",
                    "decimals": "9",
                    "price": {
                      "source": "Coingecko",
                      "updatedAt": "1679331222",
                      "dollarValuePerToken": "100.92"
                    },
                    "imageUrl": "https://sol.png"
                  },
                  "solStaked": "228895995552"
                }
              }
            },
            {
              "humanReadableDiff": "Program owner for 8eekKf..AtXzrT changed to BUfrp4..dJA75E",
              "suggestedColor": "INFO",
              "rawInfo": {
                "kind": "USER_ACCOUNT_OWNER_CHANGE",
                "data": {
                  "account": "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT",
                  "lamports": "1024632398",
                  "currentOwner": "11111111111111111111111111111111",
                  "futureOwner": "BUfrp43eBVbhc5RPsg52CDMAQKHXAj87MnZM5BdJA75E"
                }
              }
            }
          ]
        },
        "error": null
      }
    }
  )");

  auto simulation_response = solana::ParseSimulationResponse(
      ParseJson(json), "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT");
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kNone);
  EXPECT_EQ(simulation_response->warnings.size(), 0u);
  EXPECT_FALSE(simulation_response->error);
  ASSERT_EQ(simulation_response->expected_state_changes.size(), 6u);

  const auto& state_change_0 =
      simulation_response->expected_state_changes.at(0);
  EXPECT_EQ(state_change_0->human_readable_diff, "Receive 0.05657 SOL");
  EXPECT_EQ(state_change_0->suggested_color,
            mojom::BlowfishSuggestedColor::kCredit);
  EXPECT_EQ(state_change_0->raw_info->kind,
            mojom::BlowfishSolanaRawInfoKind::kSolTransfer);
  ASSERT_TRUE(state_change_0->raw_info->data->is_sol_transfer_data());
  const auto& state_change_0_raw_info =
      state_change_0->raw_info->data->get_sol_transfer_data();
  EXPECT_EQ(state_change_0_raw_info->asset->symbol, "SOL");
  EXPECT_EQ(state_change_0_raw_info->asset->name, "Solana Native Token");
  EXPECT_EQ(state_change_0_raw_info->asset->mint, "");
  EXPECT_EQ(state_change_0_raw_info->asset->decimals, 9);
  ASSERT_TRUE(state_change_0_raw_info->asset->price);
  EXPECT_EQ(state_change_0_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);
  EXPECT_EQ(state_change_0_raw_info->asset->price->last_updated_at,
            "1679331222");
  EXPECT_EQ(state_change_0_raw_info->asset->price->dollar_value_per_token,
            "100.92");
  EXPECT_EQ(state_change_0_raw_info->diff->sign,
            mojom::BlowfishDiffSign::kPlus);
  EXPECT_EQ(state_change_0_raw_info->diff->digits, "56573477");

  const auto& state_change_1 =
      simulation_response->expected_state_changes.at(1);
  EXPECT_EQ(state_change_1->human_readable_diff, "Send 2 USDT");
  EXPECT_EQ(state_change_1->suggested_color,
            mojom::BlowfishSuggestedColor::kDebit);
  EXPECT_EQ(state_change_1->raw_info->kind,
            mojom::BlowfishSolanaRawInfoKind::kSplTransfer);
  ASSERT_TRUE(state_change_1->raw_info->data->is_spl_transfer_data());
  const auto& state_change_1_raw_info =
      state_change_1->raw_info->data->get_spl_transfer_data();
  EXPECT_EQ(state_change_1_raw_info->asset->symbol, "USDT");
  EXPECT_EQ(state_change_1_raw_info->asset->name, "USDT");
  EXPECT_EQ(state_change_1_raw_info->asset->mint,
            "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB");
  EXPECT_EQ(state_change_1_raw_info->asset->decimals, 6);
  EXPECT_EQ(state_change_1_raw_info->asset->metaplex_token_standard,
            mojom::BlowfishMetaplexTokenStandardKind::kUnknown);
  ASSERT_TRUE(state_change_1_raw_info->asset->price);
  EXPECT_EQ(state_change_1_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);
  EXPECT_EQ(state_change_1_raw_info->asset->price->last_updated_at,
            "1679331222");
  EXPECT_EQ(state_change_1_raw_info->asset->price->dollar_value_per_token,
            "0.99");
  EXPECT_EQ(state_change_1_raw_info->diff->sign,
            mojom::BlowfishDiffSign::kMinus);
  EXPECT_EQ(state_change_1_raw_info->diff->digits, "2000000");
  EXPECT_EQ(state_change_1_raw_info->counterparty,
            "5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X");

  const auto& state_change_2 =
      simulation_response->expected_state_changes.at(2);
  EXPECT_EQ(state_change_2->human_readable_diff,
            "Approve to transfer Phantom QA NFT");
  EXPECT_EQ(state_change_2->suggested_color,
            mojom::BlowfishSuggestedColor::kDebit);
  EXPECT_EQ(state_change_2->raw_info->kind,
            mojom::BlowfishSolanaRawInfoKind::kSplApproval);
  ASSERT_TRUE(state_change_2->raw_info->data->is_spl_approval_data());
  const auto& state_change_2_raw_info =
      state_change_2->raw_info->data->get_spl_approval_data();
  EXPECT_EQ(state_change_2_raw_info->delegate,
            "CL38BiCb5fs3qGnKKSusaJdY24aFUUZ6vkvkujrmah83");
  EXPECT_EQ(state_change_2_raw_info->asset->symbol, "PHANTOMQA");
  EXPECT_EQ(state_change_2_raw_info->asset->name, "Phantom QA NFT");
  EXPECT_EQ(state_change_2_raw_info->asset->mint,
            "4ERKJVpwqS6Kcj115YSAjqU4WYV3YCDiYHPS72eQTY6p");
  EXPECT_EQ(state_change_2_raw_info->asset->decimals, 0);
  EXPECT_EQ(state_change_2_raw_info->asset->metaplex_token_standard,
            mojom::BlowfishMetaplexTokenStandardKind::kNonFungible);
  EXPECT_FALSE(state_change_2_raw_info->asset->price);
  EXPECT_EQ(state_change_2_raw_info->diff->sign,
            mojom::BlowfishDiffSign::kPlus);
  EXPECT_EQ(state_change_2_raw_info->diff->digits, "1525878906250000000");

  const auto& state_change_3 =
      simulation_response->expected_state_changes.at(3);
  EXPECT_EQ(state_change_3->human_readable_diff,
            "Unapprove from transferring up to 0.00132 USDC");
  EXPECT_EQ(state_change_3->suggested_color,
            mojom::BlowfishSuggestedColor::kCredit);
  EXPECT_EQ(state_change_3->raw_info->kind,
            mojom::BlowfishSolanaRawInfoKind::kSplApproval);
  ASSERT_TRUE(state_change_3->raw_info->data->is_spl_approval_data());
  const auto& state_change_3_raw_info =
      state_change_3->raw_info->data->get_spl_approval_data();
  EXPECT_EQ(state_change_3_raw_info->delegate,
            "FCRBwXC5vrzHi2Vxgn3L2NB2KKFuRhiHBjioC47sSm2o");
  EXPECT_EQ(state_change_3_raw_info->asset->symbol, "USDC");
  EXPECT_EQ(state_change_3_raw_info->asset->name, "USD Coin");
  EXPECT_EQ(state_change_3_raw_info->asset->mint,
            "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v");
  EXPECT_EQ(state_change_3_raw_info->asset->decimals, 6);
  EXPECT_EQ(state_change_3_raw_info->asset->metaplex_token_standard,
            mojom::BlowfishMetaplexTokenStandardKind::kUnknown);
  ASSERT_TRUE(state_change_3_raw_info->asset->price);
  EXPECT_EQ(state_change_3_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);
  EXPECT_EQ(state_change_3_raw_info->asset->price->last_updated_at,
            "1679331222");
  EXPECT_EQ(state_change_3_raw_info->asset->price->dollar_value_per_token,
            "1.01");
  EXPECT_EQ(state_change_3_raw_info->diff->sign,
            mojom::BlowfishDiffSign::kMinus);
  EXPECT_EQ(state_change_3_raw_info->diff->digits, "1321");

  const auto& state_change_4 =
      simulation_response->expected_state_changes.at(4);
  EXPECT_EQ(state_change_4->human_readable_diff,
            "Transfer control over your SOL staking account 2AG3be..p2vFQS");
  EXPECT_EQ(state_change_4->suggested_color,
            mojom::BlowfishSuggestedColor::kDebit);
  EXPECT_EQ(state_change_4->raw_info->kind,
            mojom::BlowfishSolanaRawInfoKind::kSolStakeAuthorityChange);
  ASSERT_TRUE(
      state_change_4->raw_info->data->is_sol_stake_authority_change_data());
  const auto& state_change_4_raw_info =
      state_change_4->raw_info->data->get_sol_stake_authority_change_data();
  EXPECT_EQ(state_change_4_raw_info->stake_account,
            "2AG3beWwvyvEMLfwJcQS9DKMV62C3UWTTf8d7gp2vFQS");
  EXPECT_EQ(state_change_4_raw_info->current_authorities->staker,
            "J58MrVr9qJPzJJS8RPQUDfaFirN3PiVHXU48zr95FY48");
  EXPECT_EQ(state_change_4_raw_info->current_authorities->withdrawer,
            "J58MrVr9qJPzJJS8RPQUDfaFirN3PiVHXU48zr95FY48");
  EXPECT_EQ(state_change_4_raw_info->future_authorities->staker,
            "EpochxXNkmM2akxBTuCEizW1oWyzgrPZ1CVZ3GpD7Egm");
  EXPECT_EQ(state_change_4_raw_info->future_authorities->withdrawer,
            "EpochxXNkmM2akxBTuCEizW1oWyzgrPZ1CVZ3GpD7Egm");
  EXPECT_EQ(state_change_4_raw_info->asset->symbol, "SOL");
  EXPECT_EQ(state_change_4_raw_info->asset->name, "Solana Native Token");
  EXPECT_EQ(state_change_4_raw_info->asset->mint, "");
  EXPECT_EQ(state_change_4_raw_info->asset->decimals, 9);
  ASSERT_TRUE(state_change_4_raw_info->asset->price);
  EXPECT_EQ(state_change_4_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);
  EXPECT_EQ(state_change_4_raw_info->asset->price->last_updated_at,
            "1679331222");
  EXPECT_EQ(state_change_4_raw_info->asset->price->dollar_value_per_token,
            "100.92");
  EXPECT_EQ(state_change_4_raw_info->sol_staked, "228895995552");

  const auto& state_change_5 =
      simulation_response->expected_state_changes.at(5);
  EXPECT_EQ(state_change_5->human_readable_diff,
            "Program owner for 8eekKf..AtXzrT changed to BUfrp4..dJA75E");
  EXPECT_EQ(state_change_5->suggested_color,
            mojom::BlowfishSuggestedColor::kInfo);
  EXPECT_EQ(state_change_5->raw_info->kind,
            mojom::BlowfishSolanaRawInfoKind::kUserAccountOwnerChange);
  ASSERT_TRUE(
      state_change_5->raw_info->data->is_user_account_owner_change_data());
  const auto& state_change_5_raw_info =
      state_change_5->raw_info->data->get_user_account_owner_change_data();
  EXPECT_EQ(state_change_5_raw_info->account,
            "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT");
  EXPECT_EQ(state_change_5_raw_info->current_owner,
            "11111111111111111111111111111111");
  EXPECT_EQ(state_change_5_raw_info->future_owner,
            "BUfrp43eBVbhc5RPsg52CDMAQKHXAj87MnZM5BdJA75E");
}

// Example adapted from
// https://docs.blowfish.xyz/reference/scan-transactions-solana
TEST(SimulationResponseParserUnitTest, ParseSolanaWarnings) {
  std::string json(R"(
    {
      "aggregated": {
        "action": "BLOCK",
        "warnings": [
          {
            "severity": "CRITICAL",
            "kind": "TRUSTED_BLOCKLIST_DOMAIN",
            "message": "This transaction originates from a known malicious domain."
          },
          {
            "severity": "WARNING",
            "kind": "SUSPECTED_MALICIOUS",
            "message": "We suspect this transaction is malicious. Approving may lead to loss of funds."
          }
        ],
        "expectedStateChanges": {
          "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT": []
        },
        "error": null
      }
    }
  )");

  auto simulation_response = solana::ParseSimulationResponse(
      ParseJson(json), "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT");
  ASSERT_TRUE(simulation_response);
  EXPECT_EQ(simulation_response->action,
            mojom::BlowfishSuggestedAction::kBlock);
  ASSERT_EQ(simulation_response->warnings.size(), 2ULL);

  const auto& warning_1 = simulation_response->warnings.at(0);
  EXPECT_EQ(warning_1->severity, mojom::BlowfishWarningSeverity::kCritical);
  EXPECT_EQ(warning_1->kind,
            mojom::BlowfishWarningKind::kTrustedBlocklistDomain);
  EXPECT_EQ(warning_1->message,
            "This transaction originates from a known malicious domain.");

  const auto& warning_2 = simulation_response->warnings.at(1);
  EXPECT_EQ(warning_2->severity, mojom::BlowfishWarningSeverity::kWarning);
  EXPECT_EQ(warning_2->kind, mojom::BlowfishWarningKind::kSuspectedMalicious);
  EXPECT_EQ(warning_2->message,
            "We suspect this transaction is malicious. Approving may lead to "
            "loss of funds.");

  EXPECT_FALSE(simulation_response->error);
  EXPECT_EQ(simulation_response->expected_state_changes.size(), 0ULL);
}

TEST(SimulationResponseParserUnitTest, ParseSolanaNullableFields) {
  std::string json_fmt(R"(
    {
      "aggregated": {
        "action": "NONE",
        "warnings": [],
        "expectedStateChanges": {
          "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT": [
            {
              "humanReadableDiff": "Send 2 USDT",
              "suggestedColor": "DEBIT",
              "rawInfo": {
                "kind": "SPL_TRANSFER",
                "data": {
                  "asset": {
                    "symbol": "USDT",
                    "name": "USDT",
                    "mint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
                    "decimals": "6",
                    "supply": "1000000000",
                    "metaplexTokenStandard": "unknown",
                    "price": null,
                    "imageUrl": "https://usdt.png"
                  },
                  "diff": {
                    "sign": "MINUS",
                    "digits": "2000000"
                  },
                  "counterparty": "5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X"
                }
              }
            }
          ]
        },
        "error": null,
      }
    }
  )");

  {
    auto json = base::StringPrintf(json_fmt.c_str(), "null");
    auto simulation_response = solana::ParseSimulationResponse(
        ParseJson(json), "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT");

    // OK: null value for asset->price is allowed
    ASSERT_TRUE(simulation_response);
    ASSERT_EQ(simulation_response->expected_state_changes.size(), 1u);

    const auto& state_change =
        simulation_response->expected_state_changes.at(0);
    const auto& state_change_raw_info =
        state_change->raw_info->data->get_spl_transfer_data();

    EXPECT_FALSE(state_change_raw_info->asset->price);
  }

  {
    auto json = base::StringPrintf(json_fmt.c_str(), "true");
    auto simulation_response = solana::ParseSimulationResponse(
        ParseJson(json), "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT");

    // OK: invalid values for nullable field asset->price are treated as
    // null.
    ASSERT_TRUE(simulation_response);
    ASSERT_EQ(simulation_response->expected_state_changes.size(), 1u);

    const auto& state_change =
        simulation_response->expected_state_changes.at(0);
    const auto& state_change_raw_info =
        state_change->raw_info->data->get_spl_transfer_data();

    EXPECT_FALSE(state_change_raw_info->asset->price);
  }

  {
    auto json = base::StringPrintf(json_fmt.c_str(), "{\"foo\": 1}");
    auto simulation_response = solana::ParseSimulationResponse(
        ParseJson(json), "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT");

    // OK: invalid dict for nullable field asset->price is treated as null.
    ASSERT_TRUE(simulation_response);
    ASSERT_EQ(simulation_response->expected_state_changes.size(), 1u);

    const auto& state_change =
        simulation_response->expected_state_changes.at(0);
    const auto& state_change_raw_info =
        state_change->raw_info->data->get_spl_transfer_data();

    EXPECT_FALSE(state_change_raw_info->asset->price);
  }
}

TEST(SimulationResponseParserUnitTest, ParseSolanaInvalidRawInfoData) {
  std::string json(R"(
    {
      "aggregated": {
        "status": "CHECKS_PASSED",
        "action": "NONE",
        "warnings": [],
        "expectedStateChanges": {
          "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT": [
            {
              "humanReadableDiff": "Send 2 USDT",
              "suggestedColor": "DEBIT",
              "rawInfo": {
                "kind": "SPL_TRANSFER",
                "data": null
              }
            }
          ]
        },
        "error": null
      }
    }
  )");

  auto simulation_response = solana::ParseSimulationResponse(
      ParseJson(json), "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT");
  EXPECT_FALSE(simulation_response);
}

TEST(SimulationResponseParserUnitTest, ParseSolanaInvalidError) {
  std::string json(R"(
    {
      "aggregated": {
        "action": "NONE",
        "warnings": [],
        "expectedStateChanges": {
          "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT": []
        },
        "error": true
      }
    }
  )");

  auto simulation_response = solana::ParseSimulationResponse(
      ParseJson(json), "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT");
  EXPECT_FALSE(simulation_response);
}

TEST(SimulationResponseParserUnitTest, ParseSolanaResponseNotDict) {
  std::string json(R"([])");

  auto simulation_response = solana::ParseSimulationResponse(
      ParseJson(json), "8eekKfUAGSJbq3CdA2TmHb8tKuyzd5gtEas3MYAtXzrT");
  EXPECT_FALSE(simulation_response);
}

}  // namespace brave_wallet
