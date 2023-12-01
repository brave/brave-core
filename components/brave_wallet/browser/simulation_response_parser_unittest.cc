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

// Example from https://docs.blowfish.xyz/reference/scan-transaction-evm
TEST(SimulationResponseParserUnitTest, ParseEVMSwapETHForDAI) {
  std::string json(R"(
    {
      "action": "NONE",
      "simulationResults": {
        "error": null,
        "gas": {
          "gasLimit": null
        },
        "expectedStateChanges": [
          {
            "humanReadableDiff": "Receive 1530.81307 DAI",
            "rawInfo": {
              "data": {
                "amount": {
                  "after": "557039306766411381864245",
                  "before": "555508493698012633714742"
                },
                "contract": {
                  "address": "0x6b175474e89094c44da98b954eedeac495271d0f",
                  "kind": "ACCOUNT"
                },
                "decimals": "18",
                "name": "Dai Stablecoin",
                "symbol": "DAI",
                "asset": {
                  "address": "0x6b175474e89094c44da98b954eedeac495271d0f",
                  "symbol": "DAI",
                  "name": "Dai Stablecoin",
                  "decimals": "18",
                  "verified": true,
                  "lists": [
                    "COINGECKO",
                    "ZERION"
                  ],
                  "imageUrl": "https://example.com/dai.png",
                  "price": {
                    "source": "Defillama",
                    "updatedAt": "1680557741",
                    "dollarValuePerToken": "1.001"
                  }
                }
              },
              "kind": "ERC20_TRANSFER"
            }
          },
          {
            "humanReadableDiff": "Send 1 ETH",
            "rawInfo": {
              "data": {
                "amount": {
                  "after": "1182957389356504134754",
                  "before": "1183957389356504134754"
                },
                "contract": {
                  "address": "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                  "kind": "ACCOUNT"
                },
                "asset": {
                  "address": "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                  "decimals": "18",
                  "imageUrl": "https://example.com/eth.png",
                  "name": "Ether",
                  "price": {
                     "dollarValuePerToken": "1968.47",
                     "source": "Coingecko",
                     "updatedAt": "1670324557"
                  },
                  "symbol": "ETH",
                  "verified": true
               }
              },
              "kind": "NATIVE_ASSET_TRANSFER"
            }
          }
        ]
      },
      "warnings": []
    }
  )");
  auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kNone);
  EXPECT_EQ(simulation_response->warnings.size(), 0u);
  EXPECT_FALSE(simulation_response->simulation_results->error);
  ASSERT_EQ(
      simulation_response->simulation_results->expected_state_changes.size(),
      2u);

  const auto& state_change_0 =
      simulation_response->simulation_results->expected_state_changes.at(0);
  EXPECT_EQ(state_change_0->human_readable_diff, "Receive 1530.81307 DAI");
  EXPECT_EQ(state_change_0->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kErc20Transfer);
  ASSERT_TRUE(state_change_0->raw_info->data->is_erc20_transfer_data());
  const auto& state_change_0_raw_info =
      state_change_0->raw_info->data->get_erc20_transfer_data();
  EXPECT_EQ(state_change_0_raw_info->amount->before,
            "555508493698012633714742");
  EXPECT_EQ(state_change_0_raw_info->amount->after, "557039306766411381864245");
  EXPECT_EQ(state_change_0_raw_info->contract->address,
            "0x6b175474e89094c44da98b954eedeac495271d0f");
  EXPECT_EQ(state_change_0_raw_info->contract->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_0_raw_info->asset->address,
            "0x6b175474e89094c44da98b954eedeac495271d0f");
  EXPECT_EQ(state_change_0_raw_info->asset->symbol, "DAI");
  EXPECT_EQ(state_change_0_raw_info->asset->name, "Dai Stablecoin");
  EXPECT_EQ(state_change_0_raw_info->asset->decimals, 18);
  EXPECT_TRUE(state_change_0_raw_info->asset->verified);
  ASSERT_EQ(state_change_0_raw_info->asset->lists.size(), 2u);
  EXPECT_EQ(state_change_0_raw_info->asset->lists.at(0), "COINGECKO");
  EXPECT_EQ(state_change_0_raw_info->asset->lists.at(1), "ZERION");
  EXPECT_EQ(state_change_0_raw_info->asset->image_url,
            "https://example.com/dai.png");
  EXPECT_EQ(state_change_0_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kDefillama);
  EXPECT_EQ(state_change_0_raw_info->asset->price->last_updated_at,
            "1680557741");
  EXPECT_EQ(state_change_0_raw_info->asset->price->dollar_value_per_token,
            "1.001");

  const auto& state_change_1 =
      simulation_response->simulation_results->expected_state_changes.at(1);
  EXPECT_EQ(state_change_1->human_readable_diff, "Send 1 ETH");
  EXPECT_EQ(state_change_1->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kNativeAssetTransfer);
  ASSERT_TRUE(state_change_1->raw_info->data->is_native_asset_transfer_data());
  const auto& state_change_1_raw_info =
      state_change_1->raw_info->data->get_native_asset_transfer_data();
  EXPECT_EQ(state_change_1_raw_info->amount->before, "1183957389356504134754");
  EXPECT_EQ(state_change_1_raw_info->amount->after, "1182957389356504134754");
  EXPECT_EQ(state_change_1_raw_info->contract->address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  EXPECT_EQ(state_change_1_raw_info->contract->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_1_raw_info->asset->address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  EXPECT_EQ(state_change_1_raw_info->asset->symbol, "ETH");
  EXPECT_EQ(state_change_1_raw_info->asset->name, "Ether");
  EXPECT_EQ(state_change_1_raw_info->asset->decimals, 18);
  EXPECT_TRUE(state_change_1_raw_info->asset->verified);
  EXPECT_EQ(state_change_1_raw_info->asset->lists.size(), 0u);
  EXPECT_EQ(state_change_1_raw_info->asset->image_url,
            "https://example.com/eth.png");
  EXPECT_EQ(state_change_1_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);
  EXPECT_EQ(state_change_1_raw_info->asset->price->last_updated_at,
            "1670324557");
  EXPECT_EQ(state_change_1_raw_info->asset->price->dollar_value_per_token,
            "1968.47");
}

// Example from https://docs.blowfish.xyz/reference/scan-transaction-evm
TEST(SimulationResponseParserUnitTest, ParseEVMERC20Approval) {
  std::string json(R"(
    {
      "action": "NONE",
      "simulationResults": {
        "error": null,
        "gas": {
          "gasLimit": null
        },
        "expectedStateChanges": [
          {
            "humanReadableDiff": "Approve to transfer up to 1000 USDT",
            "rawInfo": {
              "data": {
                "amount": {
                  "after": "1000000000",
                  "before": "0"
                },
                "asset": {
                  "address": "0xdac17f958d2ee523a2206206994597c13d831ec7",
                  "name": "Tether USD",
                  "decimals": "6",
                  "lists": [
                    "COINGECKO",
                    "ZERION"
                  ],
                  "symbol": "USDT",
                  "verified": true,
                  "imageUrl": "https://example.com/usdt.png",
                  "price": {
                    "source": "Defillama",
                    "updatedAt": "1680557741",
                    "dollarValuePerToken": "1.001"
                  }
                },
                "contract": {
                  "address": "0xdac17f958d2ee523a2206206994597c13d831ec7",
                  "kind": "ACCOUNT"
                },
                "decimals": "6",
                "name": "Tether USD",
                "owner": {
                  "address": "0xd8da6bf26964af9d7eed9e03e53415d37aa96045",
                  "kind": "ACCOUNT"
                },
                "spender": {
                  "address": "0x68b3465833fb72a70ecdf485e0e4c7bd8665fc45",
                  "kind": "ACCOUNT"
                },
                "symbol": "USDT"
              },
              "kind": "ERC20_APPROVAL"
            }
          }
        ]
      },
      "warnings": []
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kNone);
  EXPECT_EQ(simulation_response->warnings.size(), 0u);
  EXPECT_FALSE(simulation_response->simulation_results->error);
  ASSERT_EQ(
      simulation_response->simulation_results->expected_state_changes.size(),
      1u);

  const auto& state_change =
      simulation_response->simulation_results->expected_state_changes.at(0);
  EXPECT_EQ(state_change->human_readable_diff,
            "Approve to transfer up to 1000 USDT");
  EXPECT_EQ(state_change->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kErc20Approval);
  ASSERT_TRUE(state_change->raw_info->data->is_erc20_approval_data());
  const auto& state_change_raw_info =
      state_change->raw_info->data->get_erc20_approval_data();
  EXPECT_EQ(state_change_raw_info->amount->before, "0");
  EXPECT_EQ(state_change_raw_info->amount->after, "1000000000");
  EXPECT_EQ(state_change_raw_info->asset->address,
            "0xdac17f958d2ee523a2206206994597c13d831ec7");
  EXPECT_EQ(state_change_raw_info->asset->symbol, "USDT");
  EXPECT_EQ(state_change_raw_info->asset->name, "Tether USD");
  EXPECT_EQ(state_change_raw_info->asset->decimals, 6);
  EXPECT_TRUE(state_change_raw_info->asset->verified);
  ASSERT_EQ(state_change_raw_info->asset->lists.size(), 2u);
  EXPECT_EQ(state_change_raw_info->asset->lists.at(0), "COINGECKO");
  EXPECT_EQ(state_change_raw_info->asset->lists.at(1), "ZERION");
  EXPECT_EQ(state_change_raw_info->asset->image_url,
            "https://example.com/usdt.png");
  EXPECT_EQ(state_change_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kDefillama);
  EXPECT_EQ(state_change_raw_info->asset->price->last_updated_at, "1680557741");
  EXPECT_EQ(state_change_raw_info->asset->price->dollar_value_per_token,
            "1.001");
  EXPECT_EQ(state_change_raw_info->contract->address,
            "0xdac17f958d2ee523a2206206994597c13d831ec7");
  EXPECT_EQ(state_change_raw_info->contract->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->owner->address,
            "0xd8da6bf26964af9d7eed9e03e53415d37aa96045");
  EXPECT_EQ(state_change_raw_info->owner->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->spender->address,
            "0x68b3465833fb72a70ecdf485e0e4c7bd8665fc45");
  EXPECT_EQ(state_change_raw_info->spender->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
}

// Example from https://docs.blowfish.xyz/reference/scan-transaction-evm
TEST(SimulationResponseParserUnitTest, ParseEVMBuyERC721NFTWithETH) {
  std::string json(R"(
    {
      "action": "NONE",
      "warnings": [],
      "simulationResults": {
        "error": null,
        "gas": {
          "gasLimit": null
        },
        "expectedStateChanges": [
          {
            "humanReadableDiff": "Receive PudgyPenguins #7238",
            "rawInfo": {
              "kind": "ERC721_TRANSFER",
              "data": {
                "amount": {
                  "after": "1",
                  "before": "0"
                },
                "contract": {
                  "address": "0xbd3531da5cf5857e7cfaa92426877b022e612cf8",
                  "kind": "ACCOUNT"
                },
                "metadata": {
                  "rawImageUrl": "https://example.com/assets/7238.png"
                },
                "name": "PudgyPenguins",
                "symbol": "PPG",
                "tokenId": "7238",
                "assetPrice": {
                  "source": "Simplehash",
                  "updatedAt": "1679331222",
                  "dollarValuePerToken": "594.99"
                }
              }
            }
          },
          {
            "humanReadableDiff": "Send 3.181 ETH",
            "rawInfo": {
              "kind": "NATIVE_ASSET_TRANSFER",
              "data": {
                "amount": {
                  "after": "998426264937289938488",
                  "before": "1001607264937289938488"
                },
                "contract": {
                  "address": "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                  "kind": "ACCOUNT"
                },
                "asset": {
                  "address": "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                  "decimals": "18",
                  "imageUrl": "https://example.com/eth.png",
                  "name": "Ether",
                  "price": {
                     "dollarValuePerToken": "1968.47",
                     "source": "Coingecko",
                     "updatedAt": "1670324557"
                  },
                  "symbol": "ETH",
                  "verified": true
               }
              }
            }
          }
        ]
      }
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kNone);
  EXPECT_EQ(simulation_response->warnings.size(), 0u);
  EXPECT_FALSE(simulation_response->simulation_results->error);
  ASSERT_EQ(
      simulation_response->simulation_results->expected_state_changes.size(),
      2u);

  const auto& state_change_0 =
      simulation_response->simulation_results->expected_state_changes.at(0);
  EXPECT_EQ(state_change_0->human_readable_diff, "Receive PudgyPenguins #7238");
  EXPECT_EQ(state_change_0->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kErc721Transfer);
  ASSERT_TRUE(state_change_0->raw_info->data->is_erc721_transfer_data());
  const auto& state_change_0_raw_info =
      state_change_0->raw_info->data->get_erc721_transfer_data();
  EXPECT_EQ(state_change_0_raw_info->amount->before, "0");
  EXPECT_EQ(state_change_0_raw_info->amount->after, "1");
  EXPECT_EQ(state_change_0_raw_info->contract->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_0_raw_info->contract->address,
            "0xbd3531da5cf5857e7cfaa92426877b022e612cf8");
  EXPECT_EQ(state_change_0_raw_info->metadata->raw_image_url,
            "https://example.com/assets/7238.png");
  EXPECT_EQ(state_change_0_raw_info->name, "PudgyPenguins");
  EXPECT_EQ(state_change_0_raw_info->symbol, "PPG");
  EXPECT_EQ(state_change_0_raw_info->token_id, "7238");
  EXPECT_EQ(state_change_0_raw_info->asset_price->source,
            mojom::BlowfishAssetPriceSource::kSimplehash);
  EXPECT_EQ(state_change_0_raw_info->asset_price->last_updated_at,
            "1679331222");
  EXPECT_EQ(state_change_0_raw_info->asset_price->dollar_value_per_token,
            "594.99");

  const auto& state_change_1 =
      simulation_response->simulation_results->expected_state_changes.at(1);
  EXPECT_EQ(state_change_1->human_readable_diff, "Send 3.181 ETH");
  EXPECT_EQ(state_change_1->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kNativeAssetTransfer);
  ASSERT_TRUE(state_change_1->raw_info->data->is_native_asset_transfer_data());
  const auto& state_change_1_raw_info =
      state_change_1->raw_info->data->get_native_asset_transfer_data();
  EXPECT_EQ(state_change_1_raw_info->amount->before, "1001607264937289938488");
  EXPECT_EQ(state_change_1_raw_info->amount->after, "998426264937289938488");
  EXPECT_EQ(state_change_1_raw_info->contract->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_1_raw_info->contract->address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  EXPECT_EQ(state_change_1_raw_info->asset->address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  EXPECT_EQ(state_change_1_raw_info->asset->decimals, 18);
  EXPECT_EQ(state_change_1_raw_info->asset->image_url,
            "https://example.com/eth.png");
  EXPECT_EQ(state_change_1_raw_info->asset->name, "Ether");
  EXPECT_EQ(state_change_1_raw_info->asset->price->dollar_value_per_token,
            "1968.47");
  EXPECT_EQ(state_change_1_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);
  EXPECT_EQ(state_change_1_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);
  EXPECT_EQ(state_change_1_raw_info->asset->price->last_updated_at,
            "1670324557");
  EXPECT_EQ(state_change_1_raw_info->asset->symbol, "ETH");
  EXPECT_TRUE(state_change_1_raw_info->asset->verified);
}

// Example from https://docs.blowfish.xyz/reference/scan-transaction-evm
TEST(SimulationResponseParserUnitTest, ParseEVMERC721ApprovalForAll) {
  std::string json(R"(
    {
      "action": "WARN",
      "warnings": [
        {
          "kind": "UNLIMITED_ALLOWANCE_TO_NFTS",
          "message": "You are allowing this website to withdraw funds from your account in the future",
          "severity": "WARNING"
        }
      ],
      "simulationResults": {
        "error": null,
        "gas": {
          "gasLimit": null
        },
        "expectedStateChanges": [
          {
            "humanReadableDiff": "Approve to transfer all your BoredApeYachtClub",
            "rawInfo": {
              "kind": "ERC721_APPROVAL_FOR_ALL",
              "data": {
                "amount": {
                  "after": "115792089237316195423570985008687907853269984665640564039457584007913129639935",
                  "before": "0"
                },
                "contract": {
                  "address": "0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d",
                  "kind": "ACCOUNT"
                },
                "name": "BoredApeYachtClub",
                "owner": {
                  "address": "0x38191ca1307ebf67ca1a7caf5346dbd91d882ca6",
                  "kind": "ACCOUNT"
                },
                "spender": {
                  "address": "0x1e0049783f008a0085193e00003d00cd54003c71",
                  "kind": "ACCOUNT"
                },
                "symbol": "BAYC",
                "assetPrice": {
                  "source": "Simplehash",
                  "updatedAt": "1679331222",
                  "dollarValuePerToken": "7865.43"
                }
              }
            }
          }
        ]
      }
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
  ASSERT_TRUE(simulation_response);
  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kWarn);
  ASSERT_EQ(simulation_response->warnings.size(), 1u);
  EXPECT_EQ(simulation_response->warnings.at(0)->kind,
            mojom::BlowfishWarningKind::kUnlimitedAllowanceToNfts);
  EXPECT_EQ(simulation_response->warnings.at(0)->message,
            "You are allowing this website to withdraw funds from your account "
            "in the future");
  EXPECT_EQ(simulation_response->warnings.at(0)->severity,
            mojom::BlowfishWarningSeverity::kWarning);
  EXPECT_FALSE(simulation_response->simulation_results->error);
  ASSERT_EQ(
      simulation_response->simulation_results->expected_state_changes.size(),
      1u);

  const auto& state_change =
      simulation_response->simulation_results->expected_state_changes.at(0);
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
  EXPECT_EQ(state_change_raw_info->contract->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->contract->address,
            "0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d");
  EXPECT_EQ(state_change_raw_info->name, "BoredApeYachtClub");
  EXPECT_EQ(state_change_raw_info->owner->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->owner->address,
            "0x38191ca1307ebf67ca1a7caf5346dbd91d882ca6");
  EXPECT_EQ(state_change_raw_info->spender->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->spender->address,
            "0x1e0049783f008a0085193e00003d00cd54003c71");
  EXPECT_EQ(state_change_raw_info->symbol, "BAYC");
  EXPECT_EQ(state_change_raw_info->asset_price->source,
            mojom::BlowfishAssetPriceSource::kSimplehash);
  EXPECT_EQ(state_change_raw_info->asset_price->last_updated_at, "1679331222");
  EXPECT_EQ(state_change_raw_info->asset_price->dollar_value_per_token,
            "7865.43");
}

// Example from https://docs.blowfish.xyz/reference/scan-transaction-evm
TEST(SimulationResponseParserUnitTest, ParseEVMERC721Approval) {
  std::string json(R"(
    {
      "action": "NONE",
      "warnings": [],
      "simulationResults": {
        "error": null,
        "gas": {
          "gasLimit": null
        },
        "expectedStateChanges": [
          {
            "humanReadableDiff": "Approve to transfer BoredApeYachtClub",
            "rawInfo": {
              "kind": "ERC721_APPROVAL",
              "data": {
                "amount": {
                  "after": "1",
                  "before": "0"
                },
                "contract": {
                  "address": "0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d",
                  "kind": "ACCOUNT"
                },
                "metadata": {
                  "rawImageUrl": "https://example.com/6603.png"
                },
                "name": "BoredApeYachtClub",
                "owner": {
                  "address": "0xed2ab4948ba6a909a7751dec4f34f303eb8c7236",
                  "kind": "ACCOUNT"
                },
                "spender": {
                  "address": "0x1e0049783f008a0085193e00003d00cd54003c71",
                  "kind": "ACCOUNT"
                },
                "symbol": "BAYC",
                "tokenId": "6603",
                "assetPrice": {
                  "source": "Simplehash",
                  "updatedAt": "1679331222",
                  "dollarValuePerToken": "7865.43"
                }
              }
            }
          }
        ]
      }
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
  ASSERT_TRUE(simulation_response);
  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kNone);
  EXPECT_EQ(simulation_response->warnings.size(), 0u);
  EXPECT_FALSE(simulation_response->simulation_results->error);
  ASSERT_EQ(
      simulation_response->simulation_results->expected_state_changes.size(),
      1u);

  const auto& state_change =
      simulation_response->simulation_results->expected_state_changes.at(0);
  EXPECT_EQ(state_change->human_readable_diff,
            "Approve to transfer BoredApeYachtClub");
  EXPECT_EQ(state_change->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kErc721Approval);
  ASSERT_TRUE(state_change->raw_info->data->is_erc721_approval_data());
  const auto& state_change_raw_info =
      state_change->raw_info->data->get_erc721_approval_data();
  EXPECT_EQ(state_change_raw_info->amount->before, "0");
  EXPECT_EQ(state_change_raw_info->amount->after, "1");
  EXPECT_EQ(state_change_raw_info->contract->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->contract->address,
            "0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d");
  EXPECT_EQ(state_change_raw_info->metadata->raw_image_url,
            "https://example.com/6603.png");
  EXPECT_EQ(state_change_raw_info->name, "BoredApeYachtClub");
  EXPECT_EQ(state_change_raw_info->owner->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->owner->address,
            "0xed2ab4948ba6a909a7751dec4f34f303eb8c7236");
  EXPECT_EQ(state_change_raw_info->spender->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->spender->address,
            "0x1e0049783f008a0085193e00003d00cd54003c71");
  EXPECT_EQ(state_change_raw_info->symbol, "BAYC");
  EXPECT_EQ(state_change_raw_info->token_id, "6603");
  EXPECT_EQ(state_change_raw_info->asset_price->source,
            mojom::BlowfishAssetPriceSource::kSimplehash);
  EXPECT_EQ(state_change_raw_info->asset_price->last_updated_at, "1679331222");
  EXPECT_EQ(state_change_raw_info->asset_price->dollar_value_per_token,
            "7865.43");
}

// Example from https://docs.blowfish.xyz/reference/scan-transaction-evm
TEST(SimulationResponseParserUnitTest, ParseEVMBuyERC1155TokenWithETH) {
  std::string json(R"(
    {
      "action": "NONE",
      "simulationResults": {
        "error": null,
        "gas": {
          "gasLimit": null
        },
        "expectedStateChanges": [
          {
            "humanReadableDiff": "Send 0.033 ETH",
            "rawInfo": {
              "kind": "NATIVE_ASSET_TRANSFER",
              "data": {
                "amount": {
                  "after": "71057321770366572",
                  "before": "104057321770366572"
                },
                "contract": {
                  "address": "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                  "kind": "ACCOUNT"
                },
                "asset": {
                  "address": "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                  "decimals": "18",
                  "imageUrl": "https://example.com/eth.png",
                  "name": "Ether",
                  "price": {
                     "dollarValuePerToken": "1968.47",
                     "source": "Coingecko",
                     "updatedAt": "1670324557"
                  },
                  "symbol": "ETH",
                  "verified": true
               }
              }
            }
          },
          {
            "humanReadableDiff": "Receive Corgi",
            "rawInfo": {
              "kind": "ERC1155_TRANSFER",
              "data": {
                "name": "Corgi",
                "amount": {
                  "after": "1",
                  "before": "0"
                },
                "contract": {
                  "address": "0x51e613727fdd2e0b91b51c3e5427e9440a7957e4",
                  "kind": "ACCOUNT"
                },
                "metadata": {
                  "rawImageUrl": "https://example.com/13014975.png"
                },
                "tokenId": "13014975",
                "assetPrice": {
                  "source": "Simplehash",
                  "updatedAt": "1679331222",
                  "dollarValuePerToken": "232.43"
                }
              }
            }
          }
        ]
      },
      "warnings": []
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
  ASSERT_TRUE(simulation_response);
  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kNone);
  EXPECT_EQ(simulation_response->warnings.size(), 0u);
  EXPECT_FALSE(simulation_response->simulation_results->error);
  ASSERT_EQ(
      simulation_response->simulation_results->expected_state_changes.size(),
      2u);

  const auto& state_change_0 =
      simulation_response->simulation_results->expected_state_changes.at(0);
  EXPECT_EQ(state_change_0->human_readable_diff, "Send 0.033 ETH");
  EXPECT_EQ(state_change_0->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kNativeAssetTransfer);
  ASSERT_TRUE(state_change_0->raw_info->data->is_native_asset_transfer_data());
  const auto& state_change_0_raw_info =
      state_change_0->raw_info->data->get_native_asset_transfer_data();
  EXPECT_EQ(state_change_0_raw_info->amount->before, "104057321770366572");
  EXPECT_EQ(state_change_0_raw_info->amount->after, "71057321770366572");
  EXPECT_EQ(state_change_0_raw_info->contract->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_0_raw_info->contract->address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  EXPECT_EQ(state_change_0_raw_info->asset->address,
            "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
  EXPECT_EQ(state_change_0_raw_info->asset->decimals, 18);
  EXPECT_EQ(state_change_0_raw_info->asset->image_url,
            "https://example.com/eth.png");
  EXPECT_EQ(state_change_0_raw_info->asset->name, "Ether");
  EXPECT_EQ(state_change_0_raw_info->asset->price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);

  EXPECT_EQ(state_change_0_raw_info->asset->price->last_updated_at,
            "1670324557");
  EXPECT_EQ(state_change_0_raw_info->asset->price->dollar_value_per_token,
            "1968.47");
  EXPECT_EQ(state_change_0_raw_info->asset->symbol, "ETH");
  EXPECT_TRUE(state_change_0_raw_info->asset->verified);

  const auto& state_change_1 =
      simulation_response->simulation_results->expected_state_changes.at(1);
  EXPECT_EQ(state_change_1->human_readable_diff, "Receive Corgi");
  EXPECT_EQ(state_change_1->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kErc1155Transfer);
  ASSERT_TRUE(state_change_1->raw_info->data->is_erc1155_transfer_data());
  const auto& state_change_1_raw_info =
      state_change_1->raw_info->data->get_erc1155_transfer_data();
  EXPECT_EQ(state_change_1_raw_info->name, "Corgi");
  EXPECT_EQ(state_change_1_raw_info->amount->before, "0");
  EXPECT_EQ(state_change_1_raw_info->amount->after, "1");
  EXPECT_EQ(state_change_1_raw_info->contract->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_1_raw_info->contract->address,
            "0x51e613727fdd2e0b91b51c3e5427e9440a7957e4");
  EXPECT_EQ(state_change_1_raw_info->metadata->raw_image_url,
            "https://example.com/13014975.png");
  EXPECT_EQ(state_change_1_raw_info->token_id, "13014975");
  EXPECT_EQ(state_change_1_raw_info->asset_price->source,
            mojom::BlowfishAssetPriceSource::kSimplehash);
  EXPECT_EQ(state_change_1_raw_info->asset_price->last_updated_at,
            "1679331222");
  EXPECT_EQ(state_change_1_raw_info->asset_price->dollar_value_per_token,
            "232.43");
}

// Example from https://docs.blowfish.xyz/reference/scan-transaction-evm
TEST(SimulationResponseParserUnitTest, ParseEVMERC1155Transfer) {
  std::string json(R"(
    {
      "action": "NONE",
      "simulationResults": {
        "error": null,
        "gas": {
          "gasLimit": null
        },
        "expectedStateChanges": [
          {
            "humanReadableDiff": "Receive Corgi",
            "rawInfo": {
              "kind": "ERC1155_TRANSFER",
              "data": {
                "amount": {
                  "after": "1",
                  "before": "0"
                },
                "contract": {
                  "address": "0x51e613727fdd2e0b91b51c3e5427e9440a7957e4",
                  "kind": "ACCOUNT"
                },
                "metadata": {
                  "rawImageUrl": "https://example.com/13014975.png"
                },
                "tokenId": "13014975",
                "assetPrice": {
                  "source": "Simplehash",
                  "updatedAt": "1679331222",
                  "dollarValuePerToken": "232.43"
                },
                "name": %s
              }
            }
          }
        ]
      },
      "warnings": []
    }
  )");

  auto json_with_token_name = base::StringPrintf(json.c_str(), "\"Corgi\"");

  auto simulation_response =
      evm::ParseSimulationResponse(ParseJson(json_with_token_name));
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kNone);
  EXPECT_EQ(simulation_response->warnings.size(), 0u);
  EXPECT_FALSE(simulation_response->simulation_results->error);
  ASSERT_EQ(
      simulation_response->simulation_results->expected_state_changes.size(),
      1u);

  const auto& state_change =
      simulation_response->simulation_results->expected_state_changes.at(0);
  EXPECT_EQ(state_change->human_readable_diff, "Receive Corgi");
  EXPECT_EQ(state_change->raw_info->kind,
            mojom::BlowfishEVMRawInfoKind::kErc1155Transfer);
  ASSERT_TRUE(state_change->raw_info->data->is_erc1155_transfer_data());
  const auto& state_change_raw_info =
      state_change->raw_info->data->get_erc1155_transfer_data();
  EXPECT_EQ(state_change_raw_info->amount->before, "0");
  EXPECT_EQ(state_change_raw_info->amount->after, "1");
  EXPECT_EQ(state_change_raw_info->name, "Corgi");
  EXPECT_EQ(state_change_raw_info->contract->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->contract->address,
            "0x51e613727fdd2e0b91b51c3e5427e9440a7957e4");
  EXPECT_EQ(state_change_raw_info->metadata->raw_image_url,
            "https://example.com/13014975.png");
  EXPECT_EQ(state_change_raw_info->token_id, "13014975");
  EXPECT_EQ(state_change_raw_info->asset_price->source,
            mojom::BlowfishAssetPriceSource::kSimplehash);
  EXPECT_EQ(state_change_raw_info->asset_price->last_updated_at, "1679331222");
  EXPECT_EQ(state_change_raw_info->asset_price->dollar_value_per_token,
            "232.43");

  json_with_token_name = base::StringPrintf(json.c_str(), "null");
  EXPECT_FALSE(evm::ParseSimulationResponse(ParseJson(json_with_token_name)));

  json_with_token_name = base::StringPrintf(json.c_str(), "[]");
  EXPECT_FALSE(evm::ParseSimulationResponse(ParseJson(json_with_token_name)));

  json_with_token_name = base::StringPrintf(json.c_str(), "{}");
  EXPECT_FALSE(evm::ParseSimulationResponse(ParseJson(json_with_token_name)));

  json_with_token_name = base::StringPrintf(json.c_str(), "false");
  EXPECT_FALSE(evm::ParseSimulationResponse(ParseJson(json_with_token_name)));
}

// Example from https://docs.blowfish.xyz/reference/scan-transaction-evm
TEST(SimulationResponseParserUnitTest, ParseEVMERC1155ApprovalForAll) {
  std::string json(R"(
    {
      "action": "WARN",
      "warnings": [
        {
          "kind": "UNLIMITED_ALLOWANCE_TO_NFTS",
          "message": "You are allowing this website to withdraw funds from your account in the future",
          "severity": "WARNING"
        }
      ],
      "simulationResults": {
        "error": null,
        "gas": {
          "gasLimit": null
        },
        "expectedStateChanges": [
          {
            "humanReadableDiff": "Approve to transfer all your Sandbox's ASSETs",
            "rawInfo": {
              "kind": "ERC1155_APPROVAL_FOR_ALL",
              "data": {
                "amount": {
                  "after": "115792089237316195423570985008687907853269984665640564039457584007913129639935",
                  "before": "0"
                },
                "contract": {
                  "address": "0xa342f5d851e866e18ff98f351f2c6637f4478db5",
                  "kind": "ACCOUNT"
                },
                "owner": {
                  "address": "0xed2ab4948ba6a909a7751dec4f34f303eb8c7236",
                  "kind": "ACCOUNT"
                },
                "spender": {
                  "address": "0x00000000006c3852cbef3e08e8df289169ede581",
                  "kind": "ACCOUNT"
                },
                "assetPrice": {
                  "source": "Simplehash",
                  "updatedAt": "1679331222",
                  "dollarValuePerToken": "232.43"
                }
              }
            }
          }
        ]
      }
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
  ASSERT_TRUE(simulation_response);
  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kWarn);
  ASSERT_EQ(simulation_response->warnings.size(), 1u);
  EXPECT_EQ(simulation_response->warnings.at(0)->kind,
            mojom::BlowfishWarningKind::kUnlimitedAllowanceToNfts);
  EXPECT_EQ(simulation_response->warnings.at(0)->message,
            "You are allowing this website to withdraw funds from your account "
            "in the future");
  EXPECT_EQ(simulation_response->warnings.at(0)->severity,
            mojom::BlowfishWarningSeverity::kWarning);
  EXPECT_FALSE(simulation_response->simulation_results->error);
  ASSERT_EQ(
      simulation_response->simulation_results->expected_state_changes.size(),
      1u);

  const auto& state_change =
      simulation_response->simulation_results->expected_state_changes.at(0);
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
  EXPECT_EQ(state_change_raw_info->contract->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->contract->address,
            "0xa342f5d851e866e18ff98f351f2c6637f4478db5");
  EXPECT_EQ(state_change_raw_info->owner->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->owner->address,
            "0xed2ab4948ba6a909a7751dec4f34f303eb8c7236");
  EXPECT_EQ(state_change_raw_info->spender->kind,
            mojom::BlowfishEVMAddressKind::kAccount);
  EXPECT_EQ(state_change_raw_info->spender->address,
            "0x00000000006c3852cbef3e08e8df289169ede581");
  EXPECT_EQ(state_change_raw_info->asset_price->source,
            mojom::BlowfishAssetPriceSource::kSimplehash);
  EXPECT_EQ(state_change_raw_info->asset_price->last_updated_at, "1679331222");
  EXPECT_EQ(state_change_raw_info->asset_price->dollar_value_per_token,
            "232.43");
}

TEST(SimulationResponseParserUnitTest, ParseEVMSimulationErrorResponse) {
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

TEST(SimulationResponseParserUnitTest, ParseEVMInvalidDecimal) {
  std::string json(R"(
    {
      "action": "NONE",
      "simulationResults": {
        "error": null,
        "expectedStateChanges": [
          {
            "humanReadableDiff": "Send 3.181 ETH",
            "rawInfo": {
              "kind": "NATIVE_ASSET_TRANSFER",
              "data": {
                "amount": {
                  "after": "998426264937289938488",
                  "before": "1001607264937289938488"
                },
                "contract": {
                  "address": "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                  "kind": "ACCOUNT"
                },
                "asset": {
                  "address": "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                  "decimals": "boo",
                  "imageUrl": "https://example.com/eth.png",
                  "name": "Ether",
                  "price": {
                     "dollarValuePerToken": "1968.47",
                     "source": "Coingecko",
                     "updatedAt": "1670324557"
                  },
                  "symbol": "ETH",
                  "verified": true
               }
              }
            }
          }
        ],
        "gas": {
          "gasLimit": "269476"
        }
      },
      "warnings": []
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
  EXPECT_FALSE(simulation_response);
}

TEST(SimulationResponseParserUnitTest, ParseEVMUnknownError) {
  std::string json(R"(
    {
      "action": "NONE",
      "simulationResults": {
        "error": {
          "humanReadableError": "Unable to simulate transaction",
          "kind": "UNKNOWN_ERROR"
        },
        "expectedStateChanges": [],
        "gas": {
          "gasLimit": null
        }
      },
      "warnings": []
    }
  )");
  auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kNone);
  EXPECT_EQ(simulation_response->warnings.size(), 0ULL);
  ASSERT_TRUE(simulation_response->simulation_results->error);
  EXPECT_EQ(simulation_response->simulation_results->error->kind,
            mojom::BlowfishEVMErrorKind::kUnknownError);
  EXPECT_EQ(
      simulation_response->simulation_results->error->human_readable_error,
      "Unable to simulate transaction");
  EXPECT_EQ(
      simulation_response->simulation_results->expected_state_changes.size(),
      0ULL);
}

TEST(SimulationResponseParserUnitTest, ParseEVMNullableFields) {
  std::string json_fmt(R"(
    {
      "action":"NONE",
      "simulationResults":{
        "error":null,
        "expectedStateChanges":[
          {
            "humanReadableDiff":"Send 0.00307 BNB",
            "rawInfo":{
              "data":{
                "amount":{
                  "after":"90862208830306021",
                  "before":"93930808830306021"
                },
                "asset":{
                  "address":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                  "decimals":"18",
                  "imageUrl":%s,
                  "name":"Binance Coin",
                  "price":%s,
                  "symbol":"BNB",
                  "verified":true
                },
                "contract":{
                  "address":"0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
                  "kind":"ACCOUNT"
                }
              },
              "kind":"NATIVE_ASSET_TRANSFER"
            }
          }
        ],
        "gas":{
          "gasLimit":"241822"
        }
      },
      "warnings":[]
    }
  )");

  {
    auto json = base::StringPrintf(json_fmt.c_str(), "null", "null");
    auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));

    // OK: null values for asset->imageUrl and asset->price are allowed.
    ASSERT_TRUE(simulation_response);

    EXPECT_EQ(simulation_response->action,
              mojom::BlowfishSuggestedAction::kNone);
    EXPECT_EQ(simulation_response->warnings.size(), 0ULL);
    EXPECT_FALSE(simulation_response->simulation_results->error);
    ASSERT_EQ(
        simulation_response->simulation_results->expected_state_changes.size(),
        1ULL);

    const auto& state_change =
        simulation_response->simulation_results->expected_state_changes.at(0);
    EXPECT_EQ(state_change->human_readable_diff, "Send 0.00307 BNB");
    EXPECT_EQ(state_change->raw_info->kind,
              mojom::BlowfishEVMRawInfoKind::kNativeAssetTransfer);
    ASSERT_TRUE(state_change->raw_info->data->is_native_asset_transfer_data());

    const auto& state_change_raw_info =
        state_change->raw_info->data->get_native_asset_transfer_data();
    EXPECT_EQ(state_change_raw_info->amount->after, "90862208830306021");
    EXPECT_EQ(state_change_raw_info->amount->before, "93930808830306021");
    EXPECT_EQ(state_change_raw_info->contract->address,
              "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
    EXPECT_EQ(state_change_raw_info->contract->kind,
              mojom::BlowfishEVMAddressKind::kAccount);
    EXPECT_EQ(state_change_raw_info->asset->address,
              "0xeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee");
    EXPECT_EQ(state_change_raw_info->asset->decimals, 18);
    EXPECT_EQ(state_change_raw_info->asset->image_url, std::nullopt);
    EXPECT_EQ(state_change_raw_info->asset->name, "Binance Coin");
    EXPECT_FALSE(state_change_raw_info->asset->price);
    EXPECT_EQ(state_change_raw_info->asset->symbol, "BNB");
    EXPECT_TRUE(state_change_raw_info->asset->verified);
  }

  {
    auto json = base::StringPrintf(json_fmt.c_str(), "true", "null");
    // OK: invalid values for nullable field asset->imageUrl are treated as
    // null.
    auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
    ASSERT_TRUE(simulation_response);
    const auto& state_change =
        simulation_response->simulation_results->expected_state_changes.at(0);
    const auto& state_change_raw_info =
        state_change->raw_info->data->get_native_asset_transfer_data();

    EXPECT_EQ(state_change_raw_info->asset->image_url, std::nullopt);
  }

  {
    auto json = base::StringPrintf(json_fmt.c_str(), "null", "true");
    // OK: invalid values for nullable field asset->price are treated as
    // null.
    auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
    ASSERT_TRUE(simulation_response);
    const auto& state_change =
        simulation_response->simulation_results->expected_state_changes.at(0);
    const auto& state_change_raw_info =
        state_change->raw_info->data->get_native_asset_transfer_data();

    EXPECT_FALSE(state_change_raw_info->asset->price);
  }

  json_fmt = R"(
    {
      "action":"NONE",
      "simulationResults":{
        "error":null,
        "expectedStateChanges":[
          {
            "humanReadableDiff": "Receive PudgyPenguins #7238",
            "rawInfo": {
              "kind": "ERC721_TRANSFER",
              "data": {
                "amount": {
                  "after": "1",
                  "before": "0"
                },
                "contract": {
                  "address": "0xbd3531da5cf5857e7cfaa92426877b022e612cf8",
                  "kind": "ACCOUNT"
                },
                "metadata": {
                  "rawImageUrl": "https://example.com/assets/7238.png"
                },
                "name": "PudgyPenguins",
                "symbol": "PPG",
                "tokenId": "7238",
                "assetPrice":%s
              }
            }
          }
        ],
        "gas":{
          "gasLimit":"241822"
        }
      },
      "warnings":[]
    }
  )";

  {
    auto json = base::StringPrintf(json_fmt.c_str(), "null");
    // OK: null value for assetPrice field is allowed.
    auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
    ASSERT_TRUE(simulation_response);
    const auto& state_change =
        simulation_response->simulation_results->expected_state_changes.at(0);
    const auto& state_change_raw_info =
        state_change->raw_info->data->get_erc721_transfer_data();

    EXPECT_FALSE(state_change_raw_info->asset_price);
  }

  {
    auto json = base::StringPrintf(json_fmt.c_str(), "true");
    // OK: invalid values for nullable field assetPrice are treated as
    // null.
    auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
    ASSERT_TRUE(simulation_response);
    const auto& state_change =
        simulation_response->simulation_results->expected_state_changes.at(0);
    const auto& state_change_raw_info =
        state_change->raw_info->data->get_erc721_transfer_data();

    EXPECT_FALSE(state_change_raw_info->asset_price);
  }

  {
    auto json = base::StringPrintf(json_fmt.c_str(), "{\"foo\": 1}");
    // OK: invalid dict for nullable field assetPrice is treated as null.
    auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
    ASSERT_TRUE(simulation_response);
    const auto& state_change =
        simulation_response->simulation_results->expected_state_changes.at(0);
    const auto& state_change_raw_info =
        state_change->raw_info->data->get_erc721_transfer_data();

    EXPECT_FALSE(state_change_raw_info->asset_price);
  }
}

TEST(SimulationResponseParserUnitTest, ParseEVMInvalidRawInfoData) {
  std::string json(R"(
    {
      "action":"NONE",
      "simulationResults":{
        "error":null,
        "expectedStateChanges":[
          {
            "humanReadableDiff": "Receive PudgyPenguins #7238",
            "rawInfo": {
              "kind": "ERC721_TRANSFER",
              "data": null
            }
          }
        ],
        "gas":{
          "gasLimit":"241822"
        }
      },
      "warnings":[]
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
  EXPECT_FALSE(simulation_response);
}

TEST(SimulationResponseParserUnitTest, ParseEVMInvalidError) {
  std::string json(R"(
    {
      "action":"NONE",
      "simulationResults":{
        "error":false,
        "expectedStateChanges":[],
        "gas":{
          "gasLimit":"241822"
        }
      },
      "warnings":[]
    }
  )");

  auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
  EXPECT_FALSE(simulation_response);
}

TEST(SimulationResponseParserUnitTest, ParseEVMResponseNotDict) {
  std::string json(R"([])");

  auto simulation_response = evm::ParseSimulationResponse(ParseJson(json));
  EXPECT_FALSE(simulation_response);
}

// Example from https://docs.blowfish.xyz/reference/scan-transactions-solana
TEST(SimulationResponseParserUnitTest, ParseSolanaStateChanges) {
  std::string json(R"(
    {
      "status": "CHECKS_PASSED",
      "action": "NONE",
      "warnings": [],
      "simulationResults": {
        "isRecentBlockhashExpired": false,
        "expectedStateChanges": [
          {
            "humanReadableDiff": "Receive 0.05657 SOL",
            "suggestedColor": "CREDIT",
            "rawInfo": {
              "kind": "SOL_TRANSFER",
              "data": {
                "symbol": "SOL",
                "name": "Solana Native Token",
                "decimals": "9",
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
                "symbol": "USDT",
                "name": "USDT",
                "mint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
                "decimals": "6",
                "supply": "1000000000",
                "metaplexTokenStandard": "unknown",
                "assetPrice": {
                  "source": "Coingecko",
                  "last_updated_at": "1679331222",
                  "dollar_value_per_token": "0.99"
                },
                "diff": {
                  "sign": "MINUS",
                  "digits": "2000000"
                }
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
                "mint": "4ERKJVpwqS6Kcj115YSAjqU4WYV3YCDiYHPS72eQTY6p",
                "symbol": "PHANTOMQA",
                "name": "Phantom QA NFT",
                "decimals": "0",
                "diff": {
                  "sign": "PLUS",
                  "digits": "1525878906250000000"
                },
                "supply": "1",
                "metaplexTokenStandard": "non_fungible",
                "assetPrice": null
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
                "mint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                "symbol": "USDC",
                "name": "USD Coin",
                "decimals": "6",
                "diff": {
                  "sign": "MINUS",
                  "digits": "1321"
                },
                "supply": "5034964468128435",
                "metaplexTokenStandard": "unknown",
                "assetPrice": {
                  "source": "Coingecko",
                  "last_updated_at": "1679331222",
                  "dollar_value_per_token": "1.01"
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
                "currAuthorities": {
                  "staker": "J58MrVr9qJPzJJS8RPQUDfaFirN3PiVHXU48zr95FY48",
                  "withdrawer": "J58MrVr9qJPzJJS8RPQUDfaFirN3PiVHXU48zr95FY48"
                },
                "futureAuthorities": {
                  "staker": "EpochxXNkmM2akxBTuCEizW1oWyzgrPZ1CVZ3GpD7Egm",
                  "withdrawer": "EpochxXNkmM2akxBTuCEizW1oWyzgrPZ1CVZ3GpD7Egm"
                },
                "symbol": "SOL",
                "name": "Solana Native Token",
                "decimals": "9",
                "solStaked": "228895995552"
              }
            }
          }
        ],
        "error": null,
        "raw": {
          "err": null,
          "logs": [],
          "accounts": [],
          "returnData": null,
          "unitsConsumed": 148013
        }
      }
    }
  )");

  auto simulation_response = solana::ParseSimulationResponse(ParseJson(json));
  ASSERT_TRUE(simulation_response);

  EXPECT_EQ(simulation_response->action, mojom::BlowfishSuggestedAction::kNone);
  EXPECT_EQ(simulation_response->warnings.size(), 0u);
  EXPECT_FALSE(simulation_response->simulation_results->error);
  EXPECT_FALSE(
      simulation_response->simulation_results->is_recent_blockhash_expired);
  ASSERT_EQ(
      simulation_response->simulation_results->expected_state_changes.size(),
      5u);

  const auto& state_change_0 =
      simulation_response->simulation_results->expected_state_changes.at(0);
  EXPECT_EQ(state_change_0->human_readable_diff, "Receive 0.05657 SOL");
  EXPECT_EQ(state_change_0->suggested_color,
            mojom::BlowfishSuggestedColor::kCredit);
  EXPECT_EQ(state_change_0->raw_info->kind,
            mojom::BlowfishSolanaRawInfoKind::kSolTransfer);
  ASSERT_TRUE(state_change_0->raw_info->data->is_sol_transfer_data());

  const auto& state_change_0_raw_info =
      state_change_0->raw_info->data->get_sol_transfer_data();
  EXPECT_EQ(state_change_0_raw_info->symbol, "SOL");
  EXPECT_EQ(state_change_0_raw_info->name, "Solana Native Token");
  EXPECT_EQ(state_change_0_raw_info->decimals, 9);
  EXPECT_EQ(state_change_0_raw_info->diff->sign,
            mojom::BlowfishDiffSign::kPlus);
  EXPECT_EQ(state_change_0_raw_info->diff->digits, 56573477ULL);

  const auto& state_change_1 =
      simulation_response->simulation_results->expected_state_changes.at(1);
  EXPECT_EQ(state_change_1->human_readable_diff, "Send 2 USDT");
  EXPECT_EQ(state_change_1->suggested_color,
            mojom::BlowfishSuggestedColor::kDebit);
  EXPECT_EQ(state_change_1->raw_info->kind,
            mojom::BlowfishSolanaRawInfoKind::kSplTransfer);
  ASSERT_TRUE(state_change_1->raw_info->data->is_spl_transfer_data());

  const auto& state_change_1_raw_info =
      state_change_1->raw_info->data->get_spl_transfer_data();
  EXPECT_EQ(state_change_1_raw_info->symbol, "USDT");
  EXPECT_EQ(state_change_1_raw_info->name, "USDT");
  EXPECT_EQ(state_change_1_raw_info->mint,
            "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB");
  EXPECT_EQ(state_change_1_raw_info->decimals, 6);
  EXPECT_EQ(state_change_1_raw_info->supply, 1000000000ULL);
  EXPECT_EQ(state_change_1_raw_info->metaplex_token_standard,
            mojom::BlowfishMetaplexTokenStandardKind::kUnknown);
  EXPECT_EQ(state_change_1_raw_info->asset_price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);
  EXPECT_EQ(state_change_1_raw_info->asset_price->last_updated_at,
            "1679331222");
  EXPECT_EQ(state_change_1_raw_info->asset_price->dollar_value_per_token,
            "0.99");
  EXPECT_EQ(state_change_1_raw_info->diff->sign,
            mojom::BlowfishDiffSign::kMinus);
  EXPECT_EQ(state_change_1_raw_info->diff->digits, 2000000ULL);

  const auto& state_change_2 =
      simulation_response->simulation_results->expected_state_changes.at(2);
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
  EXPECT_EQ(state_change_2_raw_info->mint,
            "4ERKJVpwqS6Kcj115YSAjqU4WYV3YCDiYHPS72eQTY6p");
  EXPECT_EQ(state_change_2_raw_info->symbol, "PHANTOMQA");
  EXPECT_EQ(state_change_2_raw_info->name, "Phantom QA NFT");
  EXPECT_EQ(state_change_2_raw_info->decimals, 0);
  EXPECT_EQ(state_change_2_raw_info->diff->sign,
            mojom::BlowfishDiffSign::kPlus);
  EXPECT_EQ(state_change_2_raw_info->diff->digits, 1525878906250000000ULL);
  EXPECT_EQ(state_change_2_raw_info->supply, 1ULL);
  EXPECT_EQ(state_change_2_raw_info->metaplex_token_standard,
            mojom::BlowfishMetaplexTokenStandardKind::kNonFungible);
  EXPECT_FALSE(state_change_2_raw_info->asset_price);

  const auto& state_change_3 =
      simulation_response->simulation_results->expected_state_changes.at(3);
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
  EXPECT_EQ(state_change_3_raw_info->mint,
            "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v");
  EXPECT_EQ(state_change_3_raw_info->symbol, "USDC");
  EXPECT_EQ(state_change_3_raw_info->name, "USD Coin");
  EXPECT_EQ(state_change_3_raw_info->decimals, 6);
  EXPECT_EQ(state_change_3_raw_info->diff->sign,
            mojom::BlowfishDiffSign::kMinus);
  EXPECT_EQ(state_change_3_raw_info->diff->digits, 1321ULL);
  EXPECT_EQ(state_change_3_raw_info->supply, 5034964468128435ULL);
  EXPECT_EQ(state_change_3_raw_info->metaplex_token_standard,
            mojom::BlowfishMetaplexTokenStandardKind::kUnknown);
  EXPECT_EQ(state_change_3_raw_info->asset_price->source,
            mojom::BlowfishAssetPriceSource::kCoingecko);
  EXPECT_EQ(state_change_3_raw_info->asset_price->last_updated_at,
            "1679331222");
  EXPECT_EQ(state_change_3_raw_info->asset_price->dollar_value_per_token,
            "1.01");

  const auto& state_change_4 =
      simulation_response->simulation_results->expected_state_changes.at(4);
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
  EXPECT_EQ(state_change_4_raw_info->curr_authorities->staker,
            "J58MrVr9qJPzJJS8RPQUDfaFirN3PiVHXU48zr95FY48");
  EXPECT_EQ(state_change_4_raw_info->curr_authorities->withdrawer,
            "J58MrVr9qJPzJJS8RPQUDfaFirN3PiVHXU48zr95FY48");
  EXPECT_EQ(state_change_4_raw_info->future_authorities->withdrawer,
            "EpochxXNkmM2akxBTuCEizW1oWyzgrPZ1CVZ3GpD7Egm");
  EXPECT_EQ(state_change_4_raw_info->future_authorities->withdrawer,
            "EpochxXNkmM2akxBTuCEizW1oWyzgrPZ1CVZ3GpD7Egm");
  EXPECT_EQ(state_change_4_raw_info->symbol, "SOL");
  EXPECT_EQ(state_change_4_raw_info->name, "Solana Native Token");
  EXPECT_EQ(state_change_4_raw_info->decimals, 9);
  EXPECT_EQ(state_change_4_raw_info->sol_staked, 228895995552ULL);
}

// Example adapted from
// https://docs.blowfish.xyz/reference/scan-transactions-solana
TEST(SimulationResponseParserUnitTest, ParseSolanaWarnings) {
  std::string json(R"(
    {
      "action": "BLOCK",
      "status": "KNOWN_MALICIOUS",
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
      "simulationResults": {
        "isRecentBlockhashExpired": false,
        "expectedStateChanges": [],
        "error": null,
        "raw": {
          "err": null,
          "logs": [],
          "accounts": [],
          "returnData": null,
          "unitsConsumed": 148013
        }
      }
    }
  )");

  auto simulation_response = solana::ParseSimulationResponse(ParseJson(json));
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

  EXPECT_FALSE(simulation_response->simulation_results->error);
  EXPECT_FALSE(
      simulation_response->simulation_results->is_recent_blockhash_expired);
  EXPECT_EQ(
      simulation_response->simulation_results->expected_state_changes.size(),
      0ULL);
}

TEST(SimulationResponseParserUnitTest, ParseSolanaNullableFields) {
  std::string json_fmt(R"(
    {
      "status": "CHECKS_PASSED",
      "action": "NONE",
      "warnings": [],
      "simulationResults": {
        "isRecentBlockhashExpired": false,
        "expectedStateChanges": [
          {
            "humanReadableDiff": "Send 2 USDT",
            "suggestedColor": "DEBIT",
            "rawInfo": {
              "kind": "SPL_TRANSFER",
              "data": {
                "symbol": "USDT",
                "name": "USDT",
                "mint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
                "decimals": "6",
                "supply": "1000000000",
                "metaplexTokenStandard": "unknown",
                "assetPrice":%s,
                "diff": {
                  "sign": "MINUS",
                  "digits": "2000000"
                }
              }
            }
          }
        ],
        "error": null,
        "raw": {
          "err": null,
          "logs": [],
          "accounts": [],
          "returnData": null,
          "unitsConsumed": 148013
        }
      }
    }
  )");

  {
    auto json = base::StringPrintf(json_fmt.c_str(), "null");
    auto simulation_response = solana::ParseSimulationResponse(ParseJson(json));

    // OK: null value for assetPrice is allowed
    ASSERT_TRUE(simulation_response);

    const auto& state_change =
        simulation_response->simulation_results->expected_state_changes.at(0);
    const auto& state_change_raw_info =
        state_change->raw_info->data->get_spl_transfer_data();

    EXPECT_FALSE(state_change_raw_info->asset_price);
  }

  {
    auto json = base::StringPrintf(json_fmt.c_str(), "true");
    auto simulation_response = solana::ParseSimulationResponse(ParseJson(json));

    // OK: invalid values for nullable field assetPrice are treated as
    // null.
    ASSERT_TRUE(simulation_response);

    const auto& state_change =
        simulation_response->simulation_results->expected_state_changes.at(0);
    const auto& state_change_raw_info =
        state_change->raw_info->data->get_spl_transfer_data();

    EXPECT_FALSE(state_change_raw_info->asset_price);
  }

  {
    auto json = base::StringPrintf(json_fmt.c_str(), "{\"foo\": 1}");
    auto simulation_response = solana::ParseSimulationResponse(ParseJson(json));

    // OK: invalid dict for nullable field assetPrice is treated as null.
    ASSERT_TRUE(simulation_response);

    const auto& state_change =
        simulation_response->simulation_results->expected_state_changes.at(0);
    const auto& state_change_raw_info =
        state_change->raw_info->data->get_spl_transfer_data();

    EXPECT_FALSE(state_change_raw_info->asset_price);
  }
}

TEST(SimulationResponseParserUnitTest, ParseSolanaInvalidRawInfoData) {
  std::string json(R"(
    {
      "status": "CHECKS_PASSED",
      "action": "NONE",
      "warnings": [],
      "simulationResults": {
        "isRecentBlockhashExpired": false,
        "expectedStateChanges": [
          {
            "humanReadableDiff": "Send 2 USDT",
            "suggestedColor": "DEBIT",
            "rawInfo": {
              "kind": "SPL_TRANSFER",
              "data": null
            }
          }
        ],
        "error": null,
        "raw": {
          "err": null,
          "logs": [],
          "accounts": [],
          "returnData": null,
          "unitsConsumed": 148013
        }
      }
    }
  )");

  auto simulation_response = solana::ParseSimulationResponse(ParseJson(json));
  EXPECT_FALSE(simulation_response);
}

TEST(SimulationResponseParserUnitTest, ParseSolanaInvalidError) {
  std::string json(R"(
    {
      "status": "CHECKS_PASSED",
      "action": "NONE",
      "warnings": [],
      "simulationResults": {
        "isRecentBlockhashExpired": false,
        "expectedStateChanges": [],
        "error": true
      }
    }
  )");

  auto simulation_response = solana::ParseSimulationResponse(ParseJson(json));
  EXPECT_FALSE(simulation_response);
}

TEST(SimulationResponseParserUnitTest, ParseSolanaResponseNotDict) {
  std::string json(R"([])");

  auto simulation_response = solana::ParseSimulationResponse(ParseJson(json));
  EXPECT_FALSE(simulation_response);
}

}  // namespace brave_wallet
