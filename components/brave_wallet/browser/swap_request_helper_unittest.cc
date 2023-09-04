/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/strings/stringprintf.h"
#include "base/test/gtest_util.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/swap_request_helper.h"
#include "brave/components/brave_wallet/browser/swap_response_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;

namespace brave_wallet {
namespace {
const char* GetJupiterQuoteTemplate() {
  return R"(
    {
      "data": [
        {
          "inAmount": "%s",
          "outAmount": "261273",
          "amount": "10000",
          "otherAmountThreshold": "258660",
          "swapMode": "ExactIn",
          "priceImpactPct": "0.008955716118219659",
          "slippageBps": "50",
          "marketInfos": [
            {
              "id": "2yNwARmTmc3NzYMETCZQjAE5GGCPgviH6hiBsxaeikTK",
              "label": "Orca",
              "inputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "outputMint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
              "notEnoughLiquidity": false,
              "inAmount": "10000",
              "outAmount": "117001203",
              "priceImpactPct": "0.0000001196568750220778",
              "lpFee": {
                "amount": "%s",
                "mint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                "pct": "0.003"
              },
              "platformFee": {
                "amount": "0",
                "mint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
                "pct": "0"
              }
            }
          ]
        }
      ],
      "timeTaken": "0.044471802000089156"
    })";
}
}  // namespace

TEST(SwapRequestHelperUnitTest, EncodeJupiterTransactionParams) {
  auto* json_template = GetJupiterQuoteTemplate();
  std::string json = base::StringPrintf(json_template, "10000", "30");
  mojom::JupiterQuotePtr swap_quote = ParseJupiterQuote(ParseJson(json));
  ASSERT_TRUE(swap_quote);

  mojom::JupiterSwapParams params;
  params.route = swap_quote->routes.at(0).Clone();
  params.user_public_key = "mockPubKey";
  params.output_mint = "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";  // USDC
  auto encoded_params = EncodeJupiterTransactionParams(params.Clone(), true);

  std::string expected_params(R"(
    {
      "feeAccount": "9ogHW3wZ4unrLhQJNWnRsoggsV27QURAnb6iDptmg46j",
      "route": {
        "inAmount": 10000,
        "outAmount": 261273,
        "amount": 10000,
        "otherAmountThreshold": 258660,
        "swapMode": "ExactIn",
        "priceImpactPct": 0.008955716118219659,
        "slippageBps": 50,
        "marketInfos": [
          {
            "id": "2yNwARmTmc3NzYMETCZQjAE5GGCPgviH6hiBsxaeikTK",
            "label": "Orca",
            "inputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
            "outputMint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
            "notEnoughLiquidity": false,
            "inAmount": 10000,
            "outAmount": 117001203,
            "priceImpactPct": 1.196568750220778e-7,
            "lpFee": {
              "amount": 30,
              "mint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "pct": 0.003
            },
            "platformFee": {
              "amount": 0,
              "mint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
              "pct": 0.0
            }
          }
        ]
      },
      "userPublicKey": "mockPubKey"
    })");

  // OK: Jupiter transaction params with feeAccount
  auto expected_params_value = ParseJson(expected_params);
  ASSERT_NE(encoded_params, absl::nullopt);
  ASSERT_EQ(*encoded_params, GetJSON(expected_params_value));

  // OK: Jupiter transaction params WITHOUT feeAccount
  params.output_mint = "SHDWyBxihqiCj6YekG2GUr7wqKLeLAMK1gHZck9pL6y";  // SHDW
  encoded_params = EncodeJupiterTransactionParams(params.Clone(), false);
  expected_params = R"(
    {
      "route": {
        "inAmount": 10000,
        "outAmount": 261273,
        "amount": 10000,
        "otherAmountThreshold": 258660,
        "swapMode": "ExactIn",
        "priceImpactPct": 0.008955716118219659,
        "slippageBps": 50,
        "marketInfos": [
          {
            "id": "2yNwARmTmc3NzYMETCZQjAE5GGCPgviH6hiBsxaeikTK",
            "label": "Orca",
            "inputMint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
            "outputMint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
            "notEnoughLiquidity": false,
            "inAmount": 10000,
            "outAmount": 117001203,
            "priceImpactPct": 1.196568750220778e-7,
            "lpFee": {
              "amount": 30,
              "mint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
              "pct": 0.003
            },
            "platformFee": {
              "amount": 0,
              "mint": "MNDEFzGvMt87ueuHvVU9VcTqsAP5b3fTGPsHuuPA5ey",
              "pct": 0.0
            }
          }
        ]
      },
      "userPublicKey": "mockPubKey"
    })";
  expected_params_value = ParseJson(expected_params);
  ASSERT_NE(encoded_params, absl::nullopt);
  ASSERT_EQ(*encoded_params, GetJSON(expected_params_value));

  // KO: empty params
  EXPECT_DCHECK_DEATH(EncodeJupiterTransactionParams(nullptr, true));
  EXPECT_DCHECK_DEATH(EncodeJupiterTransactionParams(nullptr, false));

  // KO: invalid output mint
  params.output_mint = "invalid output mint";
  encoded_params = EncodeJupiterTransactionParams(params.Clone(), true);
  ASSERT_EQ(encoded_params, absl::nullopt);
  encoded_params = EncodeJupiterTransactionParams(params.Clone(), false);
  ASSERT_EQ(encoded_params, absl::nullopt);
}
}  // namespace brave_wallet
