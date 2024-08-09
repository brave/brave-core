/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/swap_request_helper.h"

#include <optional>
#include <utility>

#include "base/strings/stringprintf.h"
#include "base/test/gtest_util.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/swap_response_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;

namespace brave_wallet {

namespace {

const char* GetJupiterQuoteTemplate() {
  return R"(
    {
      "inputMint": "So11111111111111111111111111111111111111112",
      "inAmount": "100000000",
      "outputMint": "%s",
      "outAmount": "10886298",
      "otherAmountThreshold": "10885210",
      "swapMode": "ExactIn",
      "slippageBps": "1",
      "platformFee": {
        "amount": "93326",
        "feeBps": "85"
      },
      "priceImpactPct": "0",
      "routePlan": [
        {
          "swapInfo": {
            "ammKey": "EiEAydLqSKFqRPpuwYoVxEJ6h9UZh9tsTaHgs4f8b8Z5",
            "label": "Lifinity V2",
            "inputMint": "So11111111111111111111111111111111111111112",
            "outputMint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
            "inAmount": "100000000",
            "outAmount": "10964919",
            "feeAmount": "20000",
            "feeMint": "So11111111111111111111111111111111111111112"
          },
          "percent": "100"
        },
        {
          "swapInfo": {
            "ammKey": "UXD3M3N6Hn1JjbxugKguhJVHbYm8zHvdF5pNf7dumd5",
            "label": "Mercurial",
            "inputMint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
            "outputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
            "inAmount": "10964919",
            "outAmount": "10979624",
            "feeAmount": "1098",
            "feeMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v"
          },
          "percent": "100"
        }
      ]
    })";
}

const char* GetLiFiQuoteTemplate() {
  return R"(
    {
      "routes": [
        {
          "id": "0x343bc553146a3dd8438518d80bd1b6957f3fec05d137ff65a940bfb5390d3f1f",
          "fromChainId": "1",
          "fromAmountUSD": "1.00",
          "fromAmount": "1000000",
          "fromToken": {
            "address": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
            "chainId": "1",
            "symbol": "USDC",
            "decimals": "6",
            "name": "USD Coin",
            "coinKey": "USDC",
            "logoURI": "usdc.png",
            "priceUSD": "1"
          },
          "fromAddress": "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
          "toChainId": "10",
          "toAmountUSD": "1.01",
          "toAmount": "1013654",
          "toAmountMin": "1008586",
          "toToken": {
            "address": "0x94b008aA00579c1307B0EF2c499aD98a8ce58e58",
            "chainId": "10",
            "symbol": "USDT",
            "decimals": "6",
            "name": "USDT",
            "coinKey": "USDT",
            "logoURI": "usdt.png",
            "priceUSD": "0.999685"
          },
          "toAddress": "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
          "gasCostUSD": "14.65",
          "containsSwitchChain": false,
          "steps": [
            {
              "type": "lifi",
              "id": "5a6876f1-988e-4710-85b7-be2bd7681421",
              "tool": "optimism",
              "toolDetails": {
                "key": "optimism",
                "name": "Optimism Gateway",
                "logoURI": "optimism.png"
              },
              "action": {
                "fromToken": {
                  "address": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
                  "chainId": "1",
                  "symbol": "USDC",
                  "decimals": "6",
                  "name": "USD Coin",
                  "coinKey": "USDC",
                  "logoURI": "usdc.png",
                  "priceUSD": "1"
                },
                "fromAmount": "1000000",
                "toToken": {
                  "address": "0x94b008aA00579c1307B0EF2c499aD98a8ce58e58",
                  "chainId": "10",
                  "symbol": "USDT",
                  "decimals": "6",
                  "name": "USDT",
                  "coinKey": "USDT",
                  "logoURI": "usdt.png",
                  "priceUSD": "0.999685"
                },
                "fromChainId": "1",
                "toChainId": "10",
                "slippage": "0.005",
                "fromAddress": "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
                "toAddress": "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4"
              },
              "estimate": {
                "tool": "optimism",
                "approvalAddress": "0x1231DEB6f5749EF6cE6943a275A1D3E7486F4EaE",
                "toAmountMin": "1008586",
                "toAmount": "1013654",
                "fromAmount": "1000000",
                "feeCosts": [
                  {
                    "name": "LP Fee",
                    "description": "Fee paid to the Liquidity Provider",
                    "token": {
                      "address": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
                      "chainId": "1",
                      "symbol": "USDC",
                      "decimals": "6",
                      "name": "USD Coin",
                      "coinKey": "USDC",
                      "logoURI": "usdc.png",
                      "priceUSD": "1"
                    },
                    "amount": "3000",
                    "amountUSD": "0.01",
                    "percentage": "0.003",
                    "included": true
                  }
                ],
                "gasCosts": [
                  {
                    "type": "SEND",
                    "price": "17621901985",
                    "estimate": "375000",
                    "limit": "618000",
                    "amount": "6608213244375000",
                    "amountUSD": "14.65",
                    "token": {
                      "address": "0x0000000000000000000000000000000000000000",
                      "chainId": "1",
                      "symbol": "ETH",
                      "decimals": "18",
                      "name": "ETH",
                      "coinKey": "ETH",
                      "logoURI": "eth.png",
                      "priceUSD": "2216.770000000000000000"
                    }
                  }
                ],
                "executionDuration": "106.944",
                "fromAmountUSD": "1.00",
                "toAmountUSD": "1.01"
              },
              "includedSteps": [
                {
                  "id": "9ac4d9f1-9b72-463d-baf1-fcd8b09f8a8d",
                  "type": "swap",
                  "action": {
                    "fromChainId": "1",
                    "fromAmount": "1000000",
                    "fromToken": {
                      "address": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
                      "chainId": "1",
                      "symbol": "USDC",
                      "decimals": "6",
                      "name": "USD Coin",
                      "coinKey": "USDC",
                      "logoURI": "usdc.png",
                      "priceUSD": "1"
                    },
                    "toChainId": "1",
                    "toToken": {
                      "address": "0xdAC17F958D2ee523a2206206994597C13D831ec7",
                      "chainId": "1",
                      "symbol": "USDT",
                      "decimals": "6",
                      "name": "USDT",
                      "coinKey": "USDT",
                      "logoURI": "usdt.png",
                      "priceUSD": "0.999685"
                    },
                    "slippage": "0.005"
                  },
                  "estimate": {
                    "tool": "verse-dex",
                    "fromAmount": "1000000",
                    "toAmount": "1013654",
                    "toAmountMin": "1008586",
                    "approvalAddress": "0xB4B0ea46Fe0E9e8EAB4aFb765b527739F2718671",
                    "executionDuration": "30",
                    "feeCosts": [
                      {
                        "name": "LP Fee",
                        "description": "Fee paid to the Liquidity Provider",
                        "token": {
                          "address": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
                          "chainId": "1",
                          "symbol": "USDC",
                          "decimals": "6",
                          "name": "USD Coin",
                          "coinKey": "USDC",
                          "logoURI": "usdc.png",
                          "priceUSD": "1"
                        },
                        "amount": "3000",
                        "amountUSD": "0.01",
                        "percentage": "0.003",
                        "included": true
                      }
                    ],
                    "gasCosts": [
                      {
                        "type": "SEND",
                        "price": "17621901985",
                        "estimate": "200000",
                        "limit": "260000",
                        "amount": "3524380397000000",
                        "amountUSD": "7.81",
                        "token": {
                          "address": "0x0000000000000000000000000000000000000000",
                          "chainId": "1",
                          "symbol": "ETH",
                          "decimals": "18",
                          "name": "ETH",
                          "coinKey": "ETH",
                          "logoURI": "eth.png",
                          "priceUSD": "2216.770000000000000000"
                        }
                      }
                    ]
                  },
                  "tool": "verse-dex",
                  "toolDetails": {
                    "key": "verse-dex",
                    "name": "Verse Dex",
                    "logoURI": "verse.png"
                  }
                },
                {
                  "id": "b952ed38-1d5c-43bc-990a-468fd32d29b9",
                  "type": "cross",
                  "action": {
                    "fromChainId": "1",
                    "fromAmount": "1008586",
                    "fromToken": {
                      "address": "0xdAC17F958D2ee523a2206206994597C13D831ec7",
                      "chainId": "1",
                      "symbol": "USDT",
                      "decimals": "6",
                      "name": "USDT",
                      "coinKey": "USDT",
                      "logoURI": "usdt.png",
                      "priceUSD": "0.999685"
                    },
                    "toChainId": "10",
                    "toToken": {
                      "address": "0x94b008aA00579c1307B0EF2c499aD98a8ce58e58",
                      "chainId": "10",
                      "symbol": "USDT",
                      "decimals": "6",
                      "name": "USDT",
                      "coinKey": "USDT",
                      "logoURI": "usdt.png",
                      "priceUSD": "0.999685"
                    },
                    "slippage": "0.005",
                    "destinationGasConsumption": "0",
                    "destinationCallData": "0x0"
                  },
                  "estimate": {
                    "tool": "optimism",
                    "fromAmount": "1013654",
                    "toAmount": "1013654",
                    "toAmountMin": "1008586",
                    "approvalAddress": "0x99C9fc46f92E8a1c0deC1b1747d010903E884bE1",
                    "executionDuration": "76.944",
                    "feeCosts": [],
                    "gasCosts": [
                      {
                        "type": "SEND",
                        "price": "17621901985",
                        "estimate": "175000",
                        "limit": "227500",
                        "amount": "3083832847375000",
                        "amountUSD": "6.84",
                        "token": {
                          "address": "0x0000000000000000000000000000000000000000",
                          "chainId": "1",
                          "symbol": "ETH",
                          "decimals": "18",
                          "name": "ETH",
                          "coinKey": "ETH",
                          "logoURI": "eth.png",
                          "priceUSD": "2216.770000000000000000"
                        }
                      }
                    ]
                  },
                  "tool": "optimism",
                  "toolDetails": {
                    "key": "optimism",
                    "name": "Optimism Gateway",
                    "logoURI": "optimism.png"
                  }
                }
              ],
              "integrator": "jumper.exchange"
            }
          ],
          "insurance": {
            "state": "NOT_INSURABLE",
            "feeAmountUsd": "0"
          },
          "tags": [
            "RECOMMENDED",
            "CHEAPEST",
            "FASTEST"
          ]
        }
      ],
      "unavailableRoutes": {
        "filteredOut": [],
        "failed": []
      }
    }
  )";
}

const char* GetLiFiEvmToSolQuoteTemplate() {
  return R"(
    {
      "routes": [
        {
          "id": "0x9a448018e09b62da15c1bd64571c21b33cb177cee5d2f07c325d6485364362a5",
          "fromChainId": "137",
          "fromAmountUSD": "2.00",
          "fromAmount": "2000000",
          "fromToken": {
            "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
            "chainId": "137",
            "symbol": "USDCe",
            "decimals": "6",
            "name": "USDC.e",
            "coinKey": "USDCe",
            "logoURI": "eth.png",
            "priceUSD": "1"
          },
          "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
          "toChainId": "1151111081099710",
          "toAmountUSD": "1.14",
          "toAmount": "1138627",
          "toAmountMin": "1136350",
          "toToken": {
            "address": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
            "chainId": "1151111081099710",
            "symbol": "USDC",
            "decimals": "6",
            "name": "USD Coin",
            "coinKey": "USDC",
            "logoURI": "usdc.png",
            "priceUSD": "0.999501"
          },
          "toAddress": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
          "gasCostUSD": "0.02",
          "containsSwitchChain": false,
          "steps": [
            {
              "type": "lifi",
              "id": "57d247fc-d80a-4f4a-9596-72db3061aa72",
              "tool": "allbridge",
              "toolDetails": {
                "key": "allbridge",
                "name": "Allbridge",
                "logoURI": "allbridge.png"
              },
              "action": {
                "fromChainId": "137",
                "fromAmount": "2000000",
                "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
                "slippage": "0.03",
                "toChainId": "1151111081099710",
                "toAddress": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
                "fromToken": {
                  "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                  "chainId": "137",
                  "symbol": "USDCe",
                  "decimals": "6",
                  "name": "USDC.e",
                  "coinKey": "USDCe",
                  "logoURI": "usdce.png",
                  "priceUSD": "1"
                },
                "toToken": {
                  "address": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                  "chainId": "1151111081099710",
                  "symbol": "USDC",
                  "decimals": "6",
                  "name": "USD Coin",
                  "coinKey": "USDC",
                  "logoURI": "usdc.png",
                  "priceUSD": "0.999501"
                }
              },
              "estimate": {
                "tool": "allbridge",
                "fromAmount": "2000000",
                "fromAmountUSD": "2.00",
                "toAmount": "1138627",
                "toAmountMin": "1136350",
                "approvalAddress": "0x1231DEB6f5749EF6cE6943a275A1D3E7486F4EaE",
                "executionDuration": "500.298",
                "feeCosts": [
                  {
                    "name": "Allbridge's fee",
                    "description": "AllBridge fee and messenger fee charged by Allbridge",
                    "token": {
                      "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                      "chainId": "137",
                      "symbol": "USDCe",
                      "decimals": "6",
                      "name": "USDC.e",
                      "coinKey": "USDCe",
                      "logoURI": "usdce.png",
                      "priceUSD": "1"
                    },
                    "amount": "853380",
                    "amountUSD": "0.85",
                    "percentage": "0.4267",
                    "included": true
                  }
                ],
                "gasCosts": [
                  {
                    "type": "SEND",
                    "price": "112000000000",
                    "estimate": "185000",
                    "limit": "277500",
                    "amount": "20720000000000000",
                    "amountUSD": "0.02",
                    "token": {
                      "address": "0x0000000000000000000000000000000000000000",
                      "chainId": "137",
                      "symbol": "MATIC",
                      "decimals": "18",
                      "name": "MATIC",
                      "coinKey": "MATIC",
                      "logoURI": "matic.png",
                      "priceUSD": "0.809079000000000000"
                    }
                  }
                ],
                "toAmountUSD": "1.14"
              },
              "includedSteps": [
                {
                  "id": "1b914bef-e1be-4895-a9b1-c57da16d9de5",
                  "type": "cross",
                  "action": {
                    "fromChainId": "137",
                    "fromAmount": "2000000",
                    "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
                    "slippage": "0.03",
                    "toChainId": "1151111081099710",
                    "toAddress": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
                    "fromToken": {
                      "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                      "chainId": "137",
                      "symbol": "USDCe",
                      "decimals": "6",
                      "name": "USDC.e",
                      "coinKey": "USDCe",
                      "logoURI": "usdce.png",
                      "priceUSD": "1"
                    },
                    "toToken": {
                      "address": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                      "chainId": "1151111081099710",
                      "symbol": "USDC",
                      "decimals": "6",
                      "name": "USD Coin",
                      "coinKey": "USDC",
                      "logoURI": "usdc.png",
                      "priceUSD": "0.999501"
                    }
                  },
                  "estimate": {
                    "tool": "allbridge",
                    "fromAmount": "2000000",
                    "fromAmountUSD": "2.00",
                    "toAmount": "1138627",
                    "toAmountMin": "1136350",
                    "approvalAddress": "0x7775d63836987f444E2F14AA0fA2602204D7D3E0",
                    "executionDuration": "500.298",
                    "feeCosts": [
                      {
                        "name": "Allbridge's fee",
                        "description": "AllBridge fee and messenger fee charged by Allbridge",
                        "token": {
                          "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                          "chainId": "137",
                          "symbol": "USDCe",
                          "decimals": "6",
                          "name": "USDC.e",
                          "coinKey": "USDCe",
                          "logoURI": "usdce.png",
                          "priceUSD": "1"
                        },
                        "amount": "853380",
                        "amountUSD": "0.85",
                        "percentage": "0.4267",
                        "included": true
                      }
                    ],
                    "gasCosts": [
                      {
                        "type": "SEND",
                        "price": "112000000000",
                        "estimate": "185000",
                        "limit": "277500",
                        "amount": "20720000000000000",
                        "amountUSD": "0.02",
                        "token": {
                          "address": "0x0000000000000000000000000000000000000000",
                          "chainId": "137",
                          "symbol": "MATIC",
                          "decimals": "18",
                          "name": "MATIC",
                          "coinKey": "MATIC",
                          "logoURI": "matic.png",
                          "priceUSD": "0.809079000000000000"
                        }
                      }
                    ]
                  },
                  "tool": "allbridge",
                  "toolDetails": {
                    "key": "allbridge",
                    "name": "Allbridge",
                    "logoURI": "allbridge.png"
                  }
                }
              ]
            }
          ],
          "tags": [
            "RECOMMENDED",
            "CHEAPEST",
            "FASTEST"
          ],
          "insurance": {
            "state": "NOT_INSURABLE",
            "feeAmountUsd": "0"
          }
        }
      ],
      "unavailableRoutes": {
        "filteredOut": [],
        "failed": []
      }
    }
  )";
}

const char* GetLiFiEvmToSolQuoteTemplate2() {
  return R"(
    {
      "routes": [
        {
          "id": "4c901782-830f-454e-9ed8-6d246829799f",
          "fromChainId": "137",
          "fromAmountUSD": "20.00",
          "fromAmount": "20000000",
          "fromToken": {
            "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
            "chainId": "137",
            "symbol": "USDC.e",
            "decimals": "6",
            "name": "Bridged USD Coin",
            "coinKey": "USDCe",
            "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png",
            "priceUSD": "1"
          },
          "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
          "toChainId": "1151111081099710",
          "toAmountUSD": "17.15",
          "toAmount": "107802690",
          "toAmountMin": "104568610",
          "toToken": {
            "address": "11111111111111111111111111111111",
            "chainId": "1151111081099710",
            "symbol": "SOL",
            "decimals": "9",
            "name": "SOL",
            "coinKey": "SOL",
            "logoURI": "https://s2.coinmarketcap.com/static/img/coins/64x64/5426.png",
            "priceUSD": "159.11"
          },
          "toAddress": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
          "gasCostUSD": "0.01",
          "containsSwitchChain": false,
          "steps": [
            {
              "type": "lifi",
              "id": "4c901782-830f-454e-9ed8-6d246829799f:0",
              "tool": "mayan",
              "toolDetails": {
                "key": "mayan",
                "name": "Mayan",
                "logoURI": "https://raw.githubusercontent.com/lifinance/types/main/src/assets/icons/bridges/mayan.png"
              },
              "action": {
                "fromToken": {
                  "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                  "chainId": "137",
                  "symbol": "USDC.e",
                  "decimals": "6",
                  "name": "Bridged USD Coin",
                  "coinKey": "USDCe",
                  "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png",
                  "priceUSD": "1"
                },
                "fromAmount": "20000000",
                "toToken": {
                  "address": "11111111111111111111111111111111",
                  "chainId": "1151111081099710",
                  "symbol": "SOL",
                  "decimals": "9",
                  "name": "SOL",
                  "coinKey": "SOL",
                  "logoURI": "https://s2.coinmarketcap.com/static/img/coins/64x64/5426.png",
                  "priceUSD": "159.11"
                },
                "fromChainId": "137",
                "toChainId": "1151111081099710",
                "slippage": "0.03",
                "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
                "toAddress": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4"
              },
              "estimate": {
                "tool": "mayan",
                "approvalAddress": "0x1231DEB6f5749EF6cE6943a275A1D3E7486F4EaE",
                "toAmountMin": "104568610",
                "toAmount": "107802690",
                "fromAmount": "20000000",
                "feeCosts": [
                  {
                    "name": "Swap Relayer Fee",
                    "description": "Fee for the swap relayer",
                    "token": {
                      "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                      "chainId": "137",
                      "symbol": "USDC.e",
                      "decimals": "6",
                      "name": "Bridged USD Coin",
                      "coinKey": "USDCe",
                      "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png",
                      "priceUSD": "1"
                    },
                    "amount": "2746612",
                    "amountUSD": "2.75",
                    "percentage": "0.1373305975",
                    "included": true
                  }
                ],
                "gasCosts": [
                  {
                    "type": "SEND",
                    "price": "43687550986",
                    "estimate": "370000",
                    "limit": "513000",
                    "amount": "16164393864820000",
                    "amountUSD": "0.01",
                    "token": {
                      "address": "0x0000000000000000000000000000000000000000",
                      "chainId": "137",
                      "symbol": "MATIC",
                      "decimals": "18",
                      "name": "MATIC",
                      "coinKey": "MATIC",
                      "logoURI": "https://static.debank.com/image/matic_token/logo_url/matic/6f5a6b6f0732a7a235131bd7804d357c.png",
                      "priceUSD": "0.4077"
                    }
                  }
                ],
                "executionDuration": "368",
                "fromAmountUSD": "20.00",
                "toAmountUSD": "17.15"
              },
              "includedSteps": [
                {
                  "id": "e003be5c-5099-4f3a-8053-efb5767c4ba8",
                  "type": "cross",
                  "action": {
                    "fromChainId": "137",
                    "fromAmount": "20000000",
                    "fromToken": {
                      "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                      "chainId": "137",
                      "symbol": "USDC.e",
                      "decimals": "6",
                      "name": "Bridged USD Coin",
                      "coinKey": "USDCe",
                      "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png",
                      "priceUSD": "1"
                    },
                    "toChainId": "1151111081099710",
                    "toToken": {
                      "address": "11111111111111111111111111111111",
                      "chainId": "1151111081099710",
                      "symbol": "SOL",
                      "decimals": "9",
                      "name": "SOL",
                      "coinKey": "SOL",
                      "logoURI": "https://s2.coinmarketcap.com/static/img/coins/64x64/5426.png",
                      "priceUSD": "159.11"
                    },
                    "slippage": "0.03",
                    "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
                    "destinationGasConsumption": "0"
                  },
                  "estimate": {
                    "tool": "mayan",
                    "fromAmount": "20000000",
                    "toAmount": "107802690",
                    "toAmountMin": "104568610",
                    "gasCosts": [
                      {
                        "type": "SEND",
                        "price": "43687550986",
                        "estimate": "370000",
                        "limit": "555000",
                        "amount": "16164393864820000",
                        "amountUSD": "0.01",
                        "token": {
                          "address": "0x0000000000000000000000000000000000000000",
                          "chainId": "137",
                          "symbol": "MATIC",
                          "decimals": "18",
                          "name": "MATIC",
                          "coinKey": "MATIC",
                          "logoURI": "https://static.debank.com/image/matic_token/logo_url/matic/6f5a6b6f0732a7a235131bd7804d357c.png",
                          "priceUSD": "0.4077"
                        }
                      }
                    ],
                    "executionDuration": "368",
                    "approvalAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
                    "feeCosts": [
                      {
                        "name": "Swap Relayer Fee",
                        "description": "Fee for the swap relayer",
                        "token": {
                          "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                          "chainId": "137",
                          "symbol": "USDC.e",
                          "decimals": "6",
                          "name": "Bridged USD Coin",
                          "coinKey": "USDCe",
                          "logoURI": "https://raw.githubusercontent.com/trustwallet/assets/master/blockchains/ethereum/assets/0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48/logo.png",
                          "priceUSD": "1"
                        },
                        "amount": "2746612",
                        "amountUSD": "2.75",
                        "percentage": "0.1373305975",
                        "included": true
                      }
                    ]
                  },
                  "tool": "mayan",
                  "toolDetails": {
                    "key": "mayan",
                    "name": "Mayan",
                    "logoURI": "https://raw.githubusercontent.com/lifinance/types/main/src/assets/icons/bridges/mayan.png"
                  }
                }
              ],
              "integrator": "lifi-api"
            }
          ],
          "tags": [
            "RECOMMENDED",
            "CHEAPEST",
            "FASTEST"
          ]
        }
      ],
      "unavailableRoutes": {
        "filteredOut": [],
        "failed": []
      }
    }
  )";
}

}  // namespace

TEST(SwapRequestHelperUnitTest, EncodeJupiterTransactionParams) {
  auto* json_template = GetJupiterQuoteTemplate();
  std::string json = base::StringPrintf(
      json_template, "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v");  // USDC
  mojom::JupiterQuotePtr swap_quote =
      jupiter::ParseQuoteResponse(ParseJson(json));
  ASSERT_TRUE(swap_quote);

  mojom::JupiterTransactionParams params;
  params.quote = swap_quote.Clone();
  params.user_public_key = "mockPubKey";
  auto encoded_params = jupiter::EncodeTransactionParams(params);

  std::string expected_params(R"(
    {
      "feeAccount": "7mLVS86WouwN6FCv4VwgFxX4z1GtzFk1GstjQcukrAtX",
      "quoteResponse": {
        "inputMint": "So11111111111111111111111111111111111111112",
        "inAmount": "100000000",
        "outputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
        "outAmount": "10886298",
        "otherAmountThreshold": "10885210",
        "swapMode": "ExactIn",
        "slippageBps": 1,
        "platformFee": {
          "amount": "93326",
          "feeBps": 85
        },
        "priceImpactPct": "0",
        "routePlan": [
          {
            "swapInfo": {
              "ammKey": "EiEAydLqSKFqRPpuwYoVxEJ6h9UZh9tsTaHgs4f8b8Z5",
              "label": "Lifinity V2",
              "inputMint": "So11111111111111111111111111111111111111112",
              "outputMint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
              "inAmount": "100000000",
              "outAmount": "10964919",
              "feeAmount": "20000",
              "feeMint": "So11111111111111111111111111111111111111112"
            },
            "percent": 100
          },
          {
            "swapInfo": {
              "ammKey": "UXD3M3N6Hn1JjbxugKguhJVHbYm8zHvdF5pNf7dumd5",
              "label": "Mercurial",
              "inputMint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
              "outputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "inAmount": "10964919",
              "outAmount": "10979624",
              "feeAmount": "1098",
              "feeMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v"
            },
            "percent": 100
          }
        ]
      },
      "userPublicKey": "mockPubKey",
      "dynamicComputeUnitLimit": true,
      "prioritizationFeeLamports": "auto"
    })");

  // OK: Jupiter transaction params with feeAccount
  auto expected_params_value = ParseJson(expected_params);
  ASSERT_NE(encoded_params, std::nullopt);
  ASSERT_EQ(ParseJson(*encoded_params), expected_params_value);

  // OK: Jupiter transaction params WITHOUT feeAccount
  params.quote->platform_fee = nullptr;
  encoded_params = jupiter::EncodeTransactionParams(params);
  expected_params = R"(
    {
      "quoteResponse": {
        "inputMint": "So11111111111111111111111111111111111111112",
        "inAmount": "100000000",
        "outputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
        "outAmount": "10886298",
        "otherAmountThreshold": "10885210",
        "swapMode": "ExactIn",
        "slippageBps": 1,
        "platformFee": null,
        "priceImpactPct": "0",
        "routePlan": [
          {
            "swapInfo": {
              "ammKey": "EiEAydLqSKFqRPpuwYoVxEJ6h9UZh9tsTaHgs4f8b8Z5",
              "label": "Lifinity V2",
              "inputMint": "So11111111111111111111111111111111111111112",
              "outputMint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
              "inAmount": "100000000",
              "outAmount": "10964919",
              "feeAmount": "20000",
              "feeMint": "So11111111111111111111111111111111111111112"
            },
            "percent": 100
          },
          {
            "swapInfo": {
              "ammKey": "UXD3M3N6Hn1JjbxugKguhJVHbYm8zHvdF5pNf7dumd5",
              "label": "Mercurial",
              "inputMint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
              "outputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "inAmount": "10964919",
              "outAmount": "10979624",
              "feeAmount": "1098",
              "feeMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v"
            },
            "percent": 100
          }
        ]
      },
      "userPublicKey": "mockPubKey",
      "dynamicComputeUnitLimit": true,
      "prioritizationFeeLamports": "auto"
    })";
  expected_params_value = ParseJson(expected_params);
  ASSERT_NE(encoded_params, std::nullopt);
  ASSERT_EQ(ParseJson(*encoded_params), expected_params_value);

  // KO: invalid output mint
  params.quote->output_mint = "invalid output mint";
  encoded_params = jupiter::EncodeTransactionParams(params);
  ASSERT_EQ(encoded_params, std::nullopt);
  encoded_params = jupiter::EncodeTransactionParams(params);
  ASSERT_EQ(encoded_params, std::nullopt);
}

TEST(SwapRequestHelperUnitTest, EncodeLiFiQuoteParams) {
  auto params = mojom::SwapQuoteParams::New();
  params->from_account_id =
      MakeAccountId(mojom::CoinType::ETH, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived,
                    "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4");
  params->from_chain_id = mojom::kPolygonMainnetChainId;
  params->from_token = "";
  params->from_amount = "1000000000000000000000";
  params->to_account_id =
      MakeAccountId(mojom::CoinType::SOL, mojom::KeyringId::kDefault,
                    mojom::AccountKind::kDerived,
                    "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4");
  params->to_chain_id = mojom::kSolanaMainnet;
  params->to_token = "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";
  params->slippage_percentage = "3";
  params->route_priority = mojom::RoutePriority::kSafest;

  auto encoded_params = lifi::EncodeQuoteParams(std::move(params), "0.2");
  ASSERT_NE(encoded_params, std::nullopt);
  std::string expected_params(R"(
    {
      "allowDestinationCall": true,
      "fromAddress": "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
      "fromAmount": "1000000000000000000000",
      "fromChainId": "137",
      "fromTokenAddress": "0x0000000000000000000000000000000000000000",
      "options": {
        "allowSwitchChain": false,
        "fee": 0.2,
        "insurance": true,
        "integrator": "brave",
        "slippage": 0.03
      },
      "toAddress": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
      "toChainId": "1151111081099710",
      "toTokenAddress": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v"
    }
  )");
  EXPECT_EQ(ParseJson(*encoded_params), ParseJson(expected_params));
}

TEST(SwapRequestHelperUnitTest, EncodeLiFiTransactionParams) {
  // OK: EVM -> EVM bridge quotes are correctly handled
  mojom::LiFiQuotePtr quote =
      lifi::ParseQuoteResponse(ParseJson(GetLiFiQuoteTemplate()));
  ASSERT_TRUE(quote);
  ASSERT_EQ(quote->routes.size(), 1UL);
  ASSERT_EQ(quote->routes[0]->steps.size(), 1UL);

  auto& step = quote->routes[0]->steps[0];
  auto params = lifi::EncodeTransactionParams(std::move(step));
  ASSERT_NE(params, std::nullopt);

  std::string expected_params(R"(
    {
      "type": "lifi",
      "id": "5a6876f1-988e-4710-85b7-be2bd7681421",
      "tool": "optimism",
      "toolDetails": {
        "key": "optimism",
        "name": "Optimism Gateway",
        "logoURI": "optimism.png"
      },
      "action": {
        "fromToken": {
          "address": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
          "chainId": "1",
          "symbol": "USDC",
          "decimals": 6,
          "name": "USD Coin",
          "priceUSD": "0"
        },
        "fromAmount": "1000000",
        "toToken": {
          "address": "0x94b008aA00579c1307B0EF2c499aD98a8ce58e58",
          "chainId": "10",
          "symbol": "USDT",
          "decimals": 6,
          "name": "USDT",
          "priceUSD": "0"
        },
        "fromChainId": "1",
        "toChainId": "10",
        "slippage": 0.005,
        "fromAddress": "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4",
        "toAddress": "0xa92D461a9a988A7f11ec285d39783A637Fdd6ba4"
      },
      "estimate": {
        "tool": "optimism",
        "approvalAddress": "0x1231DEB6f5749EF6cE6943a275A1D3E7486F4EaE",
        "toAmountMin": "1008586",
        "toAmount": "1013654",
        "fromAmount": "1000000",
        "feeCosts": [
          {
            "name": "LP Fee",
            "description": "Fee paid to the Liquidity Provider",
            "token": {
              "address": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
              "chainId": "1",
              "symbol": "USDC",
              "decimals": 6,
              "name": "USD Coin",
              "priceUSD": "0"
            },
            "amount": "3000",
            "amountUSD": "0",
            "percentage": "0.003",
            "included": true
          }
        ],
        "gasCosts": [
          {
            "type": "SEND",
            "estimate": "375000",
            "limit": "618000",
            "amount": "6608213244375000",
            "amountUSD": "0",
            "price": "0",
            "token": {
              "address": "0x0000000000000000000000000000000000000000",
              "chainId": "1",
              "symbol": "ETH",
              "decimals": 18,
              "name": "ETH",
              "priceUSD": "0"
            }
          }
        ],
        "executionDuration": 106.944
      },
      "includedSteps": [
        {
          "id": "9ac4d9f1-9b72-463d-baf1-fcd8b09f8a8d",
          "type": "swap",
          "action": {
            "fromChainId": "1",
            "fromAmount": "1000000",
            "fromToken": {
              "address": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
              "chainId": "1",
              "symbol": "USDC",
              "decimals": 6,
              "name": "USD Coin",
              "priceUSD": "0"
            },
            "toChainId": "1",
            "toToken": {
              "address": "0xdAC17F958D2ee523a2206206994597C13D831ec7",
              "chainId": "1",
              "symbol": "USDT",
              "decimals": 6,
              "name": "USDT",
              "priceUSD": "0"
            },
            "slippage": 0.005
          },
          "estimate": {
            "tool": "verse-dex",
            "fromAmount": "1000000",
            "toAmount": "1013654",
            "toAmountMin": "1008586",
            "approvalAddress": "0xB4B0ea46Fe0E9e8EAB4aFb765b527739F2718671",
            "executionDuration": 30.0,
            "feeCosts": [
              {
                "name": "LP Fee",
                "description": "Fee paid to the Liquidity Provider",
                "token": {
                  "address": "0xA0b86991c6218b36c1d19D4a2e9Eb0cE3606eB48",
                  "chainId": "1",
                  "symbol": "USDC",
                  "decimals": 6,
                  "name": "USD Coin",
                  "priceUSD": "0"
                },
                "amount": "3000",
                "amountUSD": "0",
                "percentage": "0.003",
                "included": true
              }
            ],
            "gasCosts": [
              {
                "type": "SEND",
                "estimate": "200000",
                "limit": "260000",
                "amount": "3524380397000000",
                "amountUSD": "0",
                "price": "0",
                "token": {
                  "address": "0x0000000000000000000000000000000000000000",
                  "chainId": "1",
                  "symbol": "ETH",
                  "decimals": 18,
                  "name": "ETH",
                  "priceUSD": "0"
                }
              }
            ]
          },
          "tool": "verse-dex",
          "toolDetails": {
            "key": "verse-dex",
            "name": "Verse Dex",
            "logoURI": "verse.png"
          }
        },
        {
          "id": "b952ed38-1d5c-43bc-990a-468fd32d29b9",
          "type": "cross",
          "action": {
            "fromChainId": "1",
            "fromAmount": "1008586",
            "fromToken": {
              "address": "0xdAC17F958D2ee523a2206206994597C13D831ec7",
              "chainId": "1",
              "symbol": "USDT",
              "decimals": 6,
              "name": "USDT",
              "priceUSD": "0"
            },
            "toChainId": "10",
            "toToken": {
              "address": "0x94b008aA00579c1307B0EF2c499aD98a8ce58e58",
              "chainId": "10",
              "symbol": "USDT",
              "decimals": 6,
              "name": "USDT",
              "priceUSD": "0"
            },
            "slippage": 0.005,
            "destinationCallData": "0x0"
          },
          "estimate": {
            "tool": "optimism",
            "fromAmount": "1013654",
            "toAmount": "1013654",
            "toAmountMin": "1008586",
            "approvalAddress": "0x99C9fc46f92E8a1c0deC1b1747d010903E884bE1",
            "executionDuration": 76.944,
            "feeCosts": [],
            "gasCosts": [
              {
                "type": "SEND",
                "estimate": "175000",
                "limit": "227500",
                "amount": "3083832847375000",
                "amountUSD": "0",
                "price": "0",
                "token": {
                  "address": "0x0000000000000000000000000000000000000000",
                  "chainId": "1",
                  "symbol": "ETH",
                  "decimals": 18,
                  "name": "ETH",
                  "priceUSD": "0"
                }
              }
            ]
          },
          "tool": "optimism",
          "toolDetails": {
            "key": "optimism",
            "name": "Optimism Gateway",
            "logoURI": "optimism.png"
          }
        }
      ],
      "integrator": "jumper.exchange"
    }
  )");
  EXPECT_EQ(ParseJson(*params), ParseJson(expected_params));

  // OK: EVM -> SOL bridge quotes are correctly handled
  quote = lifi::ParseQuoteResponse(ParseJson(GetLiFiEvmToSolQuoteTemplate()));
  ASSERT_TRUE(quote);
  ASSERT_EQ(quote->routes.size(), 1UL);
  ASSERT_EQ(quote->routes[0]->steps.size(), 1UL);

  auto& step_1 = quote->routes[0]->steps[0];
  params = lifi::EncodeTransactionParams(std::move(step_1));
  ASSERT_NE(params, std::nullopt);

  expected_params = R"(
    {
      "type": "lifi",
      "id": "57d247fc-d80a-4f4a-9596-72db3061aa72",
      "tool": "allbridge",
      "toolDetails": {
        "key": "allbridge",
        "name": "Allbridge",
        "logoURI": "allbridge.png"
      },
      "action": {
        "fromChainId": "137",
        "fromAmount": "2000000",
        "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
        "slippage": 0.03,
        "toChainId": "1151111081099710",
        "toAddress": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
        "fromToken": {
          "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
          "chainId": "137",
          "symbol": "USDCe",
          "decimals": 6,
          "name": "USDC.e",
          "priceUSD": "0"
        },
        "toToken": {
          "address": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
          "chainId": "1151111081099710",
          "symbol": "USDC",
          "decimals": 6,
          "name": "USD Coin",
          "priceUSD": "0"
        }
      },
      "estimate": {
        "tool": "allbridge",
        "fromAmount": "2000000",
        "toAmount": "1138627",
        "toAmountMin": "1136350",
        "approvalAddress": "0x1231DEB6f5749EF6cE6943a275A1D3E7486F4EaE",
        "executionDuration": 500.298,
        "feeCosts": [
          {
            "name": "Allbridge's fee",
            "description": "AllBridge fee and messenger fee charged by Allbridge",
            "token": {
              "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
              "chainId": "137",
              "symbol": "USDCe",
              "decimals": 6,
              "name": "USDC.e",
              "priceUSD": "0"
            },
            "amount": "853380",
            "amountUSD": "0",
            "percentage": "0.4267",
            "included": true
          }
        ],
        "gasCosts": [
          {
            "type": "SEND",
            "estimate": "185000",
            "limit": "277500",
            "amount": "20720000000000000",
            "amountUSD": "0",
            "price": "0",
            "token": {
              "address": "0x0000000000000000000000000000000000000000",
              "chainId": "137",
              "symbol": "MATIC",
              "decimals": 18,
              "name": "MATIC",
              "priceUSD": "0"
            }
          }
        ]
      },
      "includedSteps": [
        {
          "id": "1b914bef-e1be-4895-a9b1-c57da16d9de5",
          "type": "cross",
          "action": {
            "fromChainId": "137",
            "fromAmount": "2000000",
            "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
            "slippage": 0.03,
            "toChainId": "1151111081099710",
            "toAddress": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4",
            "fromToken": {
              "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
              "chainId": "137",
              "symbol": "USDCe",
              "decimals": 6,
              "name": "USDC.e",
              "priceUSD": "0"
            },
            "toToken": {
              "address": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "chainId": "1151111081099710",
              "symbol": "USDC",
              "decimals": 6,
              "name": "USD Coin",
              "priceUSD": "0"
            }
          },
          "estimate": {
            "tool": "allbridge",
            "fromAmount": "2000000",
            "toAmount": "1138627",
            "toAmountMin": "1136350",
            "approvalAddress": "0x7775d63836987f444E2F14AA0fA2602204D7D3E0",
            "executionDuration": 500.298,
            "feeCosts": [
              {
                "name": "Allbridge's fee",
                "description": "AllBridge fee and messenger fee charged by Allbridge",
                "token": {
                  "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                  "chainId": "137",
                  "symbol": "USDCe",
                  "decimals": 6,
                  "name": "USDC.e",
                  "priceUSD": "0"
                },
                "amount": "853380",
                "amountUSD": "0",
                "percentage": "0.4267",
                "included": true
              }
            ],
            "gasCosts": [
              {
                "type": "SEND",
                "estimate": "185000",
                "limit": "277500",
                "amount": "20720000000000000",
                "amountUSD": "0",
                "price": "0",
                "token": {
                  "address": "0x0000000000000000000000000000000000000000",
                  "chainId": "137",
                  "symbol": "MATIC",
                  "decimals": 18,
                  "name": "MATIC",
                  "priceUSD": "0"
                }
              }
            ]
          },
          "tool": "allbridge",
          "toolDetails": {
            "key": "allbridge",
            "name": "Allbridge",
            "logoURI": "allbridge.png"
          }
        }
      ]
    }
  )";
  EXPECT_EQ(ParseJson(*params), ParseJson(expected_params));

  // OK: EVM to native SOL bridge quotes are correctly handled
  quote = lifi::ParseQuoteResponse(ParseJson(GetLiFiEvmToSolQuoteTemplate2()));
  ASSERT_TRUE(quote);
  ASSERT_EQ(quote->routes.size(), 1UL);
  ASSERT_EQ(quote->routes[0]->steps.size(), 1UL);

  auto& step_2 = quote->routes[0]->steps[0];
  params = lifi::EncodeTransactionParams(std::move(step_2));
  ASSERT_NE(params, std::nullopt);

  expected_params = R"(
    {
      "type": "lifi",
      "id": "4c901782-830f-454e-9ed8-6d246829799f:0",
      "tool": "mayan",
      "toolDetails": {
        "key": "mayan",
        "name": "Mayan",
        "logoURI": "https://raw.githubusercontent.com/lifinance/types/main/src/assets/icons/bridges/mayan.png"
      },
      "action": {
        "fromToken": {
          "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
          "chainId": "137",
          "symbol": "USDC.e",
          "decimals": 6,
          "name": "Bridged USD Coin",
          "priceUSD": "0"
        },
        "fromAmount": "20000000",
        "toToken": {
          "address": "11111111111111111111111111111111",
          "chainId": "1151111081099710",
          "symbol": "SOL",
          "decimals": 9,
          "name": "SOL",
          "priceUSD": "0"
        },
        "fromChainId": "137",
        "toChainId": "1151111081099710",
        "slippage": 0.03,
        "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
        "toAddress": "S5ARSDD3ddZqqqqqb2EUE2h2F1XQHBk7bErRW1WPGe4"
      },
      "estimate": {
        "tool": "mayan",
        "approvalAddress": "0x1231DEB6f5749EF6cE6943a275A1D3E7486F4EaE",
        "toAmountMin": "104568610",
        "toAmount": "107802690",
        "fromAmount": "20000000",
        "feeCosts": [
          {
            "name": "Swap Relayer Fee",
            "description": "Fee for the swap relayer",
            "token": {
              "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
              "chainId": "137",
              "symbol": "USDC.e",
              "decimals": 6,
              "name": "Bridged USD Coin",
              "priceUSD": "0"
            },
            "amount": "2746612",
            "amountUSD": "0",
            "percentage": "0.1373305975",
            "included": true
          }
        ],
        "gasCosts": [
          {
            "type": "SEND",
            "price": "0",
            "estimate": "370000",
            "limit": "513000",
            "amount": "16164393864820000",
            "amountUSD": "0",
            "token": {
              "address": "0x0000000000000000000000000000000000000000",
              "chainId": "137",
              "symbol": "MATIC",
              "decimals": 18,
              "name": "MATIC",
              "priceUSD": "0"
            }
          }
        ],
        "executionDuration": 368.0
      },
      "includedSteps": [
        {
          "id": "e003be5c-5099-4f3a-8053-efb5767c4ba8",
          "type": "cross",
          "action": {
            "fromChainId": "137",
            "fromAmount": "20000000",
            "fromToken": {
              "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
              "chainId": "137",
              "symbol": "USDC.e",
              "decimals": 6,
              "name": "Bridged USD Coin",
              "priceUSD": "0"
            },
            "toChainId": "1151111081099710",
            "toToken": {
              "address": "11111111111111111111111111111111",
              "chainId": "1151111081099710",
              "symbol": "SOL",
              "decimals": 9,
              "name": "SOL",
              "priceUSD": "0"
            },
            "slippage": 0.03,
            "fromAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0"
          },
          "estimate": {
            "tool": "mayan",
            "fromAmount": "20000000",
            "toAmount": "107802690",
            "toAmountMin": "104568610",
            "gasCosts": [
              {
                "type": "SEND",
                "price": "0",
                "estimate": "370000",
                "limit": "555000",
                "amount": "16164393864820000",
                "amountUSD": "0",
                "token": {
                  "address": "0x0000000000000000000000000000000000000000",
                  "chainId": "137",
                  "symbol": "MATIC",
                  "decimals": 18,
                  "name": "MATIC",
                  "priceUSD": "0"
                }
              }
            ],
            "executionDuration": 368.0,
            "approvalAddress": "0x552008c0f6870c2f77e5cC1d2eb9bdff03e30Ea0",
            "feeCosts": [
              {
                "name": "Swap Relayer Fee",
                "description": "Fee for the swap relayer",
                "token": {
                  "address": "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",
                  "chainId": "137",
                  "symbol": "USDC.e",
                  "decimals": 6,
                  "name": "Bridged USD Coin",
                  "priceUSD": "0"
                },
                "amount": "2746612",
                "amountUSD": "0",
                "percentage": "0.1373305975",
                "included": true
              }
            ]
          },
          "tool": "mayan",
          "toolDetails": {
            "key": "mayan",
            "name": "Mayan",
            "logoURI": "https://raw.githubusercontent.com/lifinance/types/main/src/assets/icons/bridges/mayan.png"
          }
        }
      ],
      "integrator": "lifi-api"
    }
  )";
  EXPECT_EQ(ParseJson(*params), ParseJson(expected_params));
}

}  // namespace brave_wallet
