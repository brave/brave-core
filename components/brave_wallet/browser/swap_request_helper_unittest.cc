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

}  // namespace

TEST(SwapRequestHelperUnitTest, EncodeJupiterTransactionParams) {
  auto* json_template = GetJupiterQuoteTemplate();
  std::string json = base::StringPrintf(
      json_template, "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v");  // USDC
  mojom::JupiterQuotePtr swap_quote =
      ParseJupiterQuoteResponse(ParseJson(json));
  ASSERT_TRUE(swap_quote);

  mojom::JupiterTransactionParams params;
  params.quote = swap_quote.Clone();
  params.user_public_key = "mockPubKey";
  auto encoded_params = EncodeJupiterTransactionParams(params, true);

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
      "userPublicKey": "mockPubKey"
    })");

  // OK: Jupiter transaction params with feeAccount
  auto expected_params_value = ParseJson(expected_params);
  ASSERT_NE(encoded_params, std::nullopt);
  ASSERT_EQ(*encoded_params, GetJSON(expected_params_value));

  // OK: Jupiter transaction params WITHOUT feeAccount
  params.quote->output_mint =
      "SHDWyBxihqiCj6YekG2GUr7wqKLeLAMK1gHZck9pL6y";  // SHDW
  encoded_params = EncodeJupiterTransactionParams(params, false);
  expected_params = R"(
    {
      "quoteResponse": {
        "inputMint": "So11111111111111111111111111111111111111112",
        "inAmount": "100000000",
        "outputMint": "SHDWyBxihqiCj6YekG2GUr7wqKLeLAMK1gHZck9pL6y",
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
      "userPublicKey": "mockPubKey"
    })";
  expected_params_value = ParseJson(expected_params);
  ASSERT_NE(encoded_params, std::nullopt);
  ASSERT_EQ(*encoded_params, GetJSON(expected_params_value));

  // KO: invalid output mint
  params.quote->output_mint = "invalid output mint";
  encoded_params = EncodeJupiterTransactionParams(params, true);
  ASSERT_EQ(encoded_params, std::nullopt);
  encoded_params = EncodeJupiterTransactionParams(params, false);
  ASSERT_EQ(encoded_params, std::nullopt);
}
}  // namespace brave_wallet
