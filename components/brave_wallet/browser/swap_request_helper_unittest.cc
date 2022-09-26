/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/test/gtest_util.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/swap_request_helper.h"
#include "brave/components/brave_wallet/browser/swap_response_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

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
          "outAmountWithSlippage": "258660",
          "swapMode": "ExactIn",
          "priceImpactPct": "0.008955716118219659",
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
  mojom::JupiterQuotePtr swap_quote = ParseJupiterQuote(json);
  ASSERT_TRUE(swap_quote);

  mojom::JupiterSwapParams params;
  params.route = swap_quote->routes.at(0).Clone();
  params.user_public_key = "mockPubKey";
  params.output_mint = "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v";  // USDC
  auto encoded_params = EncodeJupiterTransactionParams(params.Clone());

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

  auto expected_params_value = base::JSONReader::Read(
      expected_params,
      base::JSON_PARSE_CHROMIUM_EXTENSIONS | base::JSON_ALLOW_TRAILING_COMMAS);
  ASSERT_NE(encoded_params, absl::nullopt);
  ASSERT_EQ(*encoded_params, GetJSON(*expected_params_value));

  // Empty params
  EXPECT_DCHECK_DEATH(EncodeJupiterTransactionParams(nullptr));

  // Invalid output mint
  params.output_mint = "invalid output mint";
  encoded_params = EncodeJupiterTransactionParams(params.Clone());
  ASSERT_EQ(encoded_params, absl::nullopt);
}
}  // namespace brave_wallet
