/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_response_parser.h"

#include <optional>
#include <string>
#include <utility>

#include "base/strings/stringprintf.h"
#include "base/test/gtest_util.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;

namespace brave_wallet::solana {

TEST(SolanaResponseParserUnitTest, ParseSolanaGetBalance) {
  std::string json =
      R"({"jsonrpc":"2.0","id":1,"result":{
            "context":{"slot":106921266},"value":"18446744073709551615"}})";

  uint64_t balance = 0;
  EXPECT_TRUE(ParseGetBalance(ParseJson(json), &balance));
  EXPECT_EQ(balance, UINT64_MAX);

  json =
      R"({"jsonrpc":"2.0","id":1,"result":{
            "context":{"slot":1069},"value":"0"}})";
  EXPECT_TRUE(ParseGetBalance(ParseJson(json), &balance));
  EXPECT_EQ(balance, 0ULL);

  json =
      R"({"jsonrpc":"2.0","id":1,"result":{
            "context":{"slot":1069},"value":0}})";
  EXPECT_FALSE(ParseGetBalance(ParseJson(json), &balance));

  json =
      R"({"jsonrpc":"2.0","id":1,"result":{
            "context":{"slot":1069},"value":"63.33"}})";
  EXPECT_FALSE(ParseGetBalance(ParseJson(json), &balance));

  json =
      R"({"jsonrpc":"2.0","id":1,"result":{
            "context":{"slot":1069},"value":"-1"}})";
  EXPECT_FALSE(ParseGetBalance(ParseJson(json), &balance));
}

TEST(SolanaResponseParserUnitTest, ParseGetTokenAccountBalance) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "{\"context\":{\"slot\":1069},\"value\":{\"amount\":\"9864\", "
      "\"decimals\":2, \"uiAmount\":98.64, \"uiAmountString\":\"98.64\"}}}";

  std::string amount, ui_amount_string;
  uint8_t decimals = 0;
  EXPECT_TRUE(ParseGetTokenAccountBalance(ParseJson(json), &amount, &decimals,
                                          &ui_amount_string));
  EXPECT_EQ(amount, "9864");
  EXPECT_EQ(decimals, 2u);
  EXPECT_EQ(ui_amount_string, "98.64");

  // decimals should be uint8
  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "{\"context\":{\"slot\":1069},\"value\":{\"amount\":\"9864\", "
      "\"decimals\":256, \"uiAmount\":98.64, \"uiAmountString\":\"98.64\"}}}";
  EXPECT_FALSE(ParseGetTokenAccountBalance(ParseJson(json), &amount, &decimals,
                                           &ui_amount_string));

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "{\"context\":{\"slot\":1069},\"value\":{\"amount\":\"9864\", "
      "\"decimals\":-1, \"uiAmount\":98.64, \"uiAmountString\":\"98.64\"}}}";
  EXPECT_FALSE(ParseGetTokenAccountBalance(ParseJson(json), &amount, &decimals,
                                           &ui_amount_string));
}

TEST(SolanaResponseParserUnitTest, ParseGetSPLTokenBalances) {
  constexpr char kJsonFmt[] = R"(
    {
      "jsonrpc": "2.0",
      "result": {
        "context": {
          "apiVersion": "1.14.17",
          "slot": 195856971
        },
        "value": [
          {
            "account": {
              "data": {
                "parsed": {
                  "info": {
                    "isNative": false,
                    "mint": "7dHbWXmci3dT8UFYWYZweBLXgycu7Y3iL6trKn1Y7ARj",
                    "owner": "5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X",
                    "state": "initialized",
                    "tokenAmount": {
                      "amount": "898865",
                      "decimals": %s,
                      "uiAmount": 0.000898865,
                      "uiAmountString": "0.000898865"
                    }
                  },
                  "type": "account"
                },
                "program": "spl-token",
                "space": 165
              },
              "executable": false,
              "lamports": 2039280,
              "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
              "rentEpoch": 0
            },
            "pubkey": "5rUXc3r8bfHVadpvCUPLgcTphcwPMLihCJrxmBeaJEpR"
          },
          {
            "account": {
              "data": {
                "parsed": {
                  "info": {
                    "isNative": false,
                    "mint": "Es9vMFrzaCERmJfrF4H2FYD4KCoNkY11McCe8BenwNYB",
                    "owner": "5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X",
                    "state": "initialized",
                    "tokenAmount": {
                      "amount": "0",
                      "decimals": 6,
                      "uiAmount": 0,
                      "uiAmountString": "0"
                    }
                  },
                  "type": "account"
                },
                "program": "spl-token",
                "space": 165
              },
              "executable": false,
              "lamports": 2039280,
              "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
              "rentEpoch": 0
            },
            "pubkey": "JAzZ9e3rtXWqHfRfGuHCLQ4zQ1HgTpXpavWTkh3YJeQ2"
          },
          {
            "account": {
              "data": {
                "parsed": {
                  "info": {
                    "isNative": false,
                    "mint": "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v",
                    "owner": "5wytVPbjLb2VCXbynhUQabEZZD2B6Wxrkvwm6v6Cuy5X",
                    "state": "initialized",
                    "tokenAmount": {
                      "amount": "20508",
                      "decimals": 6,
                      "uiAmount": 0.020508,
                      "uiAmountString": "0.020508"
                    }
                  },
                  "type": "account"
                },
                "program": "spl-token",
                "space": 165
              },
              "executable": false,
              "lamports": 2039280,
              "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
              "rentEpoch": 0
            },
            "pubkey": "Hk4JYgZiuPLFc9EqqywZbtBUZyUZ8AkrSig5Nnsvvf9s"
          }
        ]
      },
      "id": 1
    }
  )";

  // OK: well-formed json
  auto json = base::StringPrintf(kJsonFmt, "9");
  auto result = ParseGetSPLTokenBalances(ParseJson(json));
  ASSERT_TRUE(result.has_value());
  ASSERT_EQ(result->size(), 2UL);

  EXPECT_EQ(result->at(0)->mint,
            "7dHbWXmci3dT8UFYWYZweBLXgycu7Y3iL6trKn1Y7ARj");
  EXPECT_EQ(result->at(0)->amount, "898865");
  EXPECT_EQ(result->at(0)->ui_amount, "0.000898865");
  EXPECT_EQ(result->at(0)->decimals, 9);

  EXPECT_EQ(result->at(1)->mint,
            "EPjFWdd5AufqSSqeM2qN1xzybapC8G4wEGGkZwyTDt1v");
  EXPECT_EQ(result->at(1)->amount, "20508");
  EXPECT_EQ(result->at(1)->ui_amount, "0.020508");
  EXPECT_EQ(result->at(1)->decimals, 6);

  // KO: decimals uint8_t overflow
  json = base::StringPrintf(kJsonFmt, "256");
  EXPECT_FALSE(ParseGetSPLTokenBalances(ParseJson(json)));

  // KO: decimals uint8_t underflow
  json = base::StringPrintf(kJsonFmt, "-1");
  EXPECT_FALSE(ParseGetSPLTokenBalances(ParseJson(json)));

  // KO: decimals type mismatch
  json = base::StringPrintf(kJsonFmt, "\"not a decimal\"");
  EXPECT_FALSE(ParseGetSPLTokenBalances(ParseJson(json)));
}

TEST(SolanaResponseParserUnitTest, ParseSendTransaction) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"2id3YC2jK9G5Wo2phDx4gJVAew8DcY5NAojnVuao8rkxwPYPe8cSwE5GzhEgJA2y8fVjDE"
      "o6iR6ykBvDxrTQrtpb\"}";
  std::string tx_id;
  EXPECT_TRUE(ParseSendTransaction(ParseJson(json), &tx_id));
  EXPECT_EQ(tx_id,
            "2id3YC2jK9G5Wo2phDx4gJVAew8DcY5NAojnVuao8rkxwPYPe8cSwE5GzhEgJA2y8f"
            "VjDEo6iR6ykBvDxrTQrtpb");
}

TEST(SolanaResponseParserUnitTest, ParseGetLatestBlockhash) {
  std::string json =
      R"({"jsonrpc":"2.0","id":1,
          "result": {
            "context":{
              "slot":1069
            },
            "value": {
              "blockhash":"EkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N",
              "lastValidBlockHeight": "18446744073709551615"
            }
          }
         })";
  std::string hash;
  uint64_t last_valid_block_height = 0;
  EXPECT_TRUE(ParseGetLatestBlockhash(ParseJson(json), &hash,
                                      &last_valid_block_height));
  EXPECT_EQ(hash, "EkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N");
  EXPECT_EQ(last_valid_block_height, UINT64_MAX);

  std::vector<std::string> invalid_jsons = {
      R"({"jsonrpc":"2.0", "id":1})",
      R"({"jsonrpc":"2.0", "id":1, "result":{}})",
      R"({"jsonrpc":"2.0", "id":1, "result":{"value":{}}})",
      R"({"jsonrpc":"2.0", "id":1, "result":{"value":{"blockhash":""}}})",
      R"({"jsonrpc":"2.0", "id":1, "result":{"value":{"blockhash":"EkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N"}}})",
      R"({"jsonrpc":"2.0", "id":1, "result":{"value":{"lastValidBlockHeight": "3090"}}})",
  };
  for (const auto& invalid_json : invalid_jsons) {
    EXPECT_FALSE(ParseGetLatestBlockhash(ParseJson(invalid_json), &hash,
                                         &last_valid_block_height))
        << invalid_json;
  }
}

TEST(SolanaResponseParserUnitTest, ParseGetSignatureStatuses) {
  std::string json = R"(
      {"jsonrpc":2.0, "id":1, "result":
        {
          "context": {"slot": 82},
          "value": [
            {
              "slot": "18446744073709551615",
              "confirmations": "10",
              "err": null,
              "confirmationStatus": "confirmed"
            },
            {
              "slot": "72",
              "confirmations": "18446744073709551615",
              "err": null,
              "confirmationStatus": "confirmed"
            },
            {
              "slot": "1092",
              "confirmations": null,
              "err": {"InstructionError":[0,{"Custom":1}]},
              "confirmationStatus": "finalized"
            },
            {
              "slot": "11",
              "confirmations": "0",
              "err": null,
              "confirmationStatus": null
            },
            null
          ]
        }
      }
  )";

  std::vector<std::optional<SolanaSignatureStatus>> statuses;
  ASSERT_TRUE(ParseGetSignatureStatuses(ParseJson(json), &statuses));

  std::vector<std::optional<SolanaSignatureStatus>> expected_statuses(
      {SolanaSignatureStatus(UINT64_MAX, 10u, "", "confirmed"),
       SolanaSignatureStatus(72u, UINT64_MAX, "", "confirmed"),
       SolanaSignatureStatus(
           1092u, 0u, R"({"InstructionError":[0,{"Custom":1}]})", "finalized"),
       SolanaSignatureStatus(11u, 0u, "", ""), std::nullopt});

  EXPECT_EQ(statuses, expected_statuses);

  std::string invalid = R"(
      {"jsonrpc":2.0, "id":1, "result":
        {
          "context": {"slot": 82},
          "value": [
            {
              "slot": "18446744073709551615",
              "confirmations": 10,
              "err": null,
              "confirmationStatus": "confirmed"
            },
            {
              "slot": 72,
              "confirmations": "18446744073709551615",
              "err": null,
              "confirmationStatus": "confirmed"
            },
            {},
            []
          ]
        }
      }
  )";
  expected_statuses =
      std::vector<std::optional<SolanaSignatureStatus>>(4, std::nullopt);
  ASSERT_TRUE(ParseGetSignatureStatuses(ParseJson(invalid), &statuses));
  EXPECT_EQ(expected_statuses, statuses);
}

TEST(SolanaResponseParserUnitTest, ParseGetAccountInfo) {
  std::string json = R"(
    {
      "jsonrpc":"2.0","id":1,
      "result": {
        "context":{"slot":123065869},
        "value":{
          "data":["SEVMTE8gV09STEQ=","base64"],
          "executable":false,
          "lamports":"18446744073709551615",
          "owner":"11111111111111111111111111111111",
          "rentEpoch":"18446744073709551615"
        }
      }
    }
  )";

  SolanaAccountInfo expected_info;
  expected_info.lamports = UINT64_MAX;
  expected_info.owner = "11111111111111111111111111111111";
  expected_info.data = "SEVMTE8gV09STEQ=";
  expected_info.executable = false;
  expected_info.rent_epoch = UINT64_MAX;

  std::optional<SolanaAccountInfo> info;
  ASSERT_TRUE(ParseGetAccountInfo(ParseJson(json), &info));
  EXPECT_EQ(*info, expected_info);

  json = R"(
    {
      "jsonrpc":"2.0","id":1,
      "result":{
        "context":{"slot":123121238},
        "value":null
      }
    })";
  ASSERT_TRUE(ParseGetAccountInfo(ParseJson(json), &info));
  EXPECT_FALSE(info);

  // Parsing should fail if data string is not base64 encoded as it says.
  json = R"(
    {
      "jsonrpc":"2.0","id":1,
      "result": {
        "context":{"slot":123065869},
        "value":{
          "data":["JvSKSz9YHfqEQ8j","base64"],
          "executable":false,
          "lamports":"88801034809120",
          "owner":"11111111111111111111111111111111",
          "rentEpoch":"284"
        }
      }
    }
  )";
  EXPECT_FALSE(ParseGetAccountInfo(ParseJson(json), &info));

  // data using base58 is not supported.
  json = R"(
    {
      "jsonrpc":"2.0","id":1,
      "result": {
        "context":{"slot":123065869},
        "value":{
          "data":["JvSKSz9YHfqEQ8j","base58"],
          "executable":false,
          "lamports":"88801034809120",
          "owner":"11111111111111111111111111111111",
          "rentEpoch":"284"
        }
      }
    }
  )";
  EXPECT_FALSE(ParseGetAccountInfo(ParseJson(json), &info));

  // data using jsonParsed encoding param is not supported.
  json = R"(
    {
      "jsonrpc":"2.0","id":1,
      "result": {
        "context":{"slot":123065869},
        "value":{
          "data":{
            "nonce": {
              "initialized": {
                "authority": "Bbqg1M4YVVfbhEzwA9SpC9FhsaG83YMTYoR4a8oTDLX",
                "blockhash": "3xLP3jK6dVJwpeGeTDYTwdDK3TKchUf1gYYGHa4sF3XJ",
                "feeCalculator": {
                  "lamportsPerSignature": 5000
                }
              }
            }
          },
          "executable":false,
          "lamports":"88801034809120",
          "owner":"11111111111111111111111111111111",
          "rentEpoch":"284"
        }
      }
    }
  )";
  EXPECT_FALSE(ParseGetAccountInfo(ParseJson(json), &info));

  json = R"({"jsonrpc":"2.0","id":1,"result":{"value":{}}})";
  EXPECT_FALSE(ParseGetAccountInfo(ParseJson(json), &info));

  EXPECT_DCHECK_DEATH(ParseGetAccountInfo(ParseJson(json), nullptr));
}

TEST(SolanaResponseParserUnitTest, ParseGetFeeForMessage) {
  uint64_t fee = 0u;
  std::string json =
      R"({"jsonrpc":"2.0", "id":1, "result":{"value":"18446744073709551615"}})";
  EXPECT_TRUE(ParseGetFeeForMessage(ParseJson(json), &fee));
  EXPECT_EQ(fee, UINT64_MAX);

  EXPECT_TRUE(ParseGetFeeForMessage(
      ParseJson(R"({"jsonrpc":"2.0", "id":1, "result":{"value":null}})"),
      &fee));
  EXPECT_EQ(fee, 0u);

  std::vector<std::string> invalid_jsons = {
      R"({"jsonrpc":"2.0", "id":1})",
      R"({"jsonrpc":"2.0", "id":1, "result":{}})",
      R"({"jsonrpc":"2.0", "id":1, "result":{"value":{}}})"};
  for (const auto& invalid_json : invalid_jsons) {
    EXPECT_FALSE(ParseGetFeeForMessage(ParseJson(invalid_json), &fee))
        << invalid_json;
  }

  EXPECT_DCHECK_DEATH(ParseGetFeeForMessage(ParseJson(json), nullptr));
}

TEST(SolanaResponseParserUnitTest, ParseGetBlockHeight) {
  std::string json =
      R"({"jsonrpc":"2.0","id":1,"result":"18446744073709551615"})";

  uint64_t block_height = 0;
  EXPECT_TRUE(ParseGetBlockHeight(ParseJson(json), &block_height));
  EXPECT_EQ(block_height, UINT64_MAX);

  std::vector<std::string> invalid_jsons = {
      R"({"jsonrpc":"2.0", "id":1})",
      R"({"jsonrpc":"2.0", "id":1, "result":{}})",
      R"({"jsonrpc":"2.0", "id":1, "result":null})",
      R"({"jsonrpc":"2.0", "id":1, "result":-1})",
      R"({"jsonrpc":"2.0", "id":1, "result":1.2})",
      R"({"jsonrpc":"2.0", "id":1, "result":1})"};
  for (const auto& invalid_json : invalid_jsons) {
    EXPECT_FALSE(ParseGetBlockHeight(ParseJson(invalid_json), &block_height))
        << invalid_json;
  }

  EXPECT_DCHECK_DEATH(ParseGetBlockHeight(ParseJson(json), nullptr));
}

TEST(SolanaResponseParserUnitTest, ParseIsBlockhashValid) {
  auto value = base::test::ParseJson(
      R"({"jsonrpc": "2.0",
          "id": 1,
          "result": {
            "context": {
              "slot": 2483
            },
            "value": false
          }})");
  auto is_valid = ParseIsBlockhashValid(value);
  ASSERT_TRUE(is_valid);
  EXPECT_FALSE(*is_valid);

  std::vector<std::string> invalid_jsons = {
      R"({"jsonrpc":"2.0", "id":1})",
      R"({"jsonrpc":"2.0", "id":1, "result":{}})",
      R"({"jsonrpc":"2.0", "id":1, "result":{"value":{}}})",
      R"({"jsonrpc":"2.0", "id":1, "result":{"value":1}})",
      R"({"jsonrpc":"2.0", "id":1, "result":{"value":{"is_valid":true}}})"};
  for (const auto& invalid_json : invalid_jsons) {
    EXPECT_FALSE(ParseIsBlockhashValid(ParseJson(invalid_json)))
        << invalid_json;
  }
}

TEST(SolanaResponseParserUnitTest, ParseGetTokenAccountsByOwner) {
  std::string json = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.14.10",
        "slot": 166492586
      },
      "value": [
        {
          "account": {
            "data": [
              "afxiYbRCtH5HgLYFzytARQOXmFT6HhvNzk2Baxua+lM2kEWUG3BArj8SJRSnd1faFt2Tm0Ey/qtGnPdOOlQlugEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": "2039280",
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": "361"
          },
          "pubkey": "81ZdQjbr7FhEPmcyGJtG8BAUyWxAjb2iSiWFEQn8i8Da"
        },
        {
          "account": {
            "data": [
              "z6cxAUoRHIupvmezOL4EAsTLlwKTgwxzCg/xcNWSEu42kEWUG3BArj8SJRSnd1faFt2Tm0Ey/qtGnPdOOlQlugEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA",
              "base64"
            ],
            "executable": false,
            "lamports": "2039280",
            "owner": "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA",
            "rentEpoch": "361"
          },
          "pubkey": "5gjGaTE41sPVS1Dzwg43ipdj9NTtApZLcK55ihRuVb6Y"
        }
      ]
    },
    "id": 1
  })";

  std::vector<SolanaAccountInfo> token_accounts;
  EXPECT_TRUE(ParseGetTokenAccountsByOwner(ParseJson(json), &token_accounts));
  EXPECT_EQ(token_accounts.size(), 2u);
}

TEST(SolanaResponseParserUnitTest, ConverterForGetAccountInfo) {
  std::string json = R"(
    {
      "jsonrpc":"2.0","id":1,
      "result": {
        "context":{"slot":123065869},
        "value":{
          "data":["SEVMTE8gV09STEQ=","base64"],
          "executable":false,
          "lamports":18446744073709551615,
          "owner":"11111111111111111111111111111111",
          "rentEpoch":18446744073709551615
        }
      }
    }
  )";
  // 'lamports' and 'rentEpoch' are converted to strings.
  std::string json_expected = R"(
    {
      "jsonrpc":"2.0","id":1,
      "result": {
        "context":{"slot":123065869},
        "value":{
          "data":["SEVMTE8gV09STEQ=","base64"],
          "executable":false,
          "lamports":"18446744073709551615",
          "owner":"11111111111111111111111111111111",
          "rentEpoch":"18446744073709551615"
        }
      }
    }
  )";
  auto json_converted = ConverterForGetAccountInfo().Run(json);
  ASSERT_TRUE(json_converted);
  EXPECT_EQ(ParseJson(*json_converted), ParseJson(json_expected));
}

TEST(SolanaResponseParserUnitTest, ConverterForGetProgramAccounts) {
  std::string json = R"(
    {
      "jsonrpc": "2.0",
      "result": [
        {
          "account": {
            "data": "2R9jLfiAQ9bgdcw6h8s44439",
            "executable": false,
            "lamports": 18446744073709551615,
            "owner": "4Nd1mBQtrMJVYVfKf2PJy9NZUZdTAsp7D4xWLs4gDB4T",
            "rentEpoch": 18446744073709551615
          },
          "pubkey": "CxELquR1gPP8wHe33gZ4QxqGB3sZ9RSwsJ2KshVewkFY"
        }
      ],
      "id": 1
    }
  )";
  // 'lamports' and 'rentEpoch' are converted to strings.
  std::string json_expected = R"(
    {
      "jsonrpc": "2.0",
      "result": [
        {
          "account": {
            "data": "2R9jLfiAQ9bgdcw6h8s44439",
            "executable": false,
            "lamports": "18446744073709551615",
            "owner": "4Nd1mBQtrMJVYVfKf2PJy9NZUZdTAsp7D4xWLs4gDB4T",
            "rentEpoch": "18446744073709551615"
          },
          "pubkey": "CxELquR1gPP8wHe33gZ4QxqGB3sZ9RSwsJ2KshVewkFY"
        }
      ],
      "id": 1
    }
  )";
  auto json_converted = ConverterForGetProgramAccounts().Run(json);
  ASSERT_TRUE(json_converted);
  EXPECT_EQ(ParseJson(*json_converted), ParseJson(json_expected));

  json = R"(
    {
      "jsonrpc": "2.0",
      "result": [
        {
          "account": {
            "data": "2R9jLfiAQ9bgdcw6h8s44439",
            "executable": false,
            "lamports": 123,
            "owner": "4Nd1mBQtrMJVYVfKf2PJy9NZUZdTAsp7D4xWLs4gDB4T",
            "rentEpoch": 123
          },
          "pubkey": "CxELquR1gPP8wHe33gZ4QxqGB3sZ9RSwsJ2KshVewkFY"
        },
        {
          "account": {
            "data": "2R9jLfiAQ9bgdcw6h8s44439",
            "executable": false,
            "lamports": 123,
            "owner": "4Nd1mBQtrMJVYVfKf2PJy9NZUZdTAsp7D4xWLs4gDB4T",
            "rentEpoch": 123
          },
          "pubkey": "CxELquR1gPP8wHe33gZ4QxqGB3sZ9RSwsJ2KshVewkFY"
        }
      ],
      "id": 1
    }
  )";
  // 'lamports' and 'rentEpoch' for the first item are converted to strings.
  json_expected = R"(
    {
      "jsonrpc": "2.0",
      "result": [
        {
          "account": {
            "data": "2R9jLfiAQ9bgdcw6h8s44439",
            "executable": false,
            "lamports": "123",
            "owner": "4Nd1mBQtrMJVYVfKf2PJy9NZUZdTAsp7D4xWLs4gDB4T",
            "rentEpoch": "123"
          },
          "pubkey": "CxELquR1gPP8wHe33gZ4QxqGB3sZ9RSwsJ2KshVewkFY"
        },
        {
          "account": {
            "data": "2R9jLfiAQ9bgdcw6h8s44439",
            "executable": false,
            "lamports": 123,
            "owner": "4Nd1mBQtrMJVYVfKf2PJy9NZUZdTAsp7D4xWLs4gDB4T",
            "rentEpoch": 123
          },
          "pubkey": "CxELquR1gPP8wHe33gZ4QxqGB3sZ9RSwsJ2KshVewkFY"
        }
      ],
      "id": 1
    }
  )";
  json_converted = ConverterForGetProgramAccounts().Run(json);
  ASSERT_TRUE(json_converted);
  EXPECT_EQ(ParseJson(*json_converted), ParseJson(json_expected));

  json = R"(
    {
      "jsonrpc": "2.0",
      "result": [
      ],
      "id": 1
    }
  )";
  // Empty result case.
  json_expected = R"(
    {
      "jsonrpc": "2.0",
      "result": [
      ],
      "id": 1
    }
  )";
  json_converted = ConverterForGetProgramAccounts().Run(json);
  ASSERT_TRUE(json_converted);
  EXPECT_EQ(ParseJson(*json_converted), ParseJson(json_expected));
}

TEST(SolanaResponseParserUnitTest, ParseSimulateTransaction) {
  // Test with a valid JSON string that includes a unitsConsumed field.
  std::string json_valid = R"(
    {
      "jsonrpc": "2.0",
      "result": {
        "context": {
          "apiVersion": "1.17.25",
          "slot": 259225005
        },
        "value": {
          "accounts": null,
          "err": null,
          "logs": [
            "Program BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY invoke [1]",
            "Program noopb9bkMVfRPU8AsbpTUg8AQkHtKwMYZiFUjNRtMmV consumed 39 of 183791 compute units",
            "Program noopb9bkMVfRPU8AsbpTUg8AQkHtKwMYZiFUjNRtMmV success"
          ],
          "returnData": null,
          "unitsConsumed": "18446744073709551615"
        }
      },
      "id": 1
    }
  )";

  auto parsed_units_valid = ParseSimulateTransaction(ParseJson(json_valid));
  ASSERT_TRUE(parsed_units_valid.has_value());
  EXPECT_EQ(*parsed_units_valid, UINT64_MAX);

  // Test with a JSON string that lacks the unitsConsumed field.
  std::string json_no_units = R"(
    {
      "jsonrpc": "2.0",
      "result": {
        "context": {
          "apiVersion": "1.17.25",
          "slot": 259225005
        },
        "value": {
          "accounts": null,
          "err": null,
          "logs": [
            "Program BGUMAp9Gq7iTEuizy4pqaxsTyUCBK68MDfK752saRPUY invoke [1]",
            "Program noopb9bkMVfRPU8AsbpTUg8AQkHtKwMYZiFUjNRtMmV consumed 39 of 183791 compute units",
            "Program noopb9bkMVfRPU8AsbpTUg8AQkHtKwMYZiFUjNRtMmV success"
          ],
          "returnData": null
        }
      },
      "id": 1
    }
  )";

  auto parsed_no_units = ParseSimulateTransaction(ParseJson(json_no_units));
  EXPECT_FALSE(parsed_no_units.has_value());

  // Test with invalid JSON (e.g., missing result).
  std::string json_invalid = R"(
    {
      "jsonrpc": "2.0",
      "id": 1
    }
  )";

  auto parsed_invalid = ParseSimulateTransaction(ParseJson(json_invalid));
  EXPECT_FALSE(parsed_invalid.has_value());

  // Test with blockhash not found error
  std::string json_blockhash_err = R"({
    "jsonrpc": "2.0",
    "result": {
      "context": {
        "apiVersion": "1.18.11",
        "slot": 262367830
      },
      "value": {
        "accounts": null,
        "err": "BlockhashNotFound",
        "innerInstructions": null,
        "logs": [],
        "returnData": null,
        "unitsConsumed": "0"
      }
    },
    "id": 1
  })";
  auto parsed_blockhash_err =
      ParseSimulateTransaction(ParseJson(json_blockhash_err));
  EXPECT_FALSE(parsed_blockhash_err.has_value());
}

TEST(SolanaResponseParserUnitTest, ParseGetSolanaPrioritizationFees) {
  // Parsing valid JSON with prioritization fees
  std::string json = R"({
      "jsonrpc": "2.0",
      "result": [
        {
          "slot": "348125",
          "prioritizationFee": "0"
        },
        {
          "slot": "348126",
          "prioritizationFee": "1000"
        },
        {
          "slot": "348127",
          "prioritizationFee": "500"
        },
        {
          "slot": "348128",
          "prioritizationFee": "0"
        },
        {
          "slot": "18446744073709551615",
          "prioritizationFee": "18446744073709551615"
        }
      ],
      "id": 1
    })";

  std::optional<std::vector<std::pair<uint64_t, uint64_t>>> fees =
      ParseGetSolanaPrioritizationFees(ParseJson(json));
  ASSERT_TRUE(fees.has_value());

  std::vector<std::pair<uint64_t, uint64_t>> expected_fees = {
      {348125, 0},
      {348126, 1000},
      {348127, 500},
      {348128, 0},
      {UINT64_MAX, UINT64_MAX}};

  EXPECT_EQ(*fees, expected_fees);

  // Testing invalid JSON without 'result' key
  std::string invalid_json = R"(
    {
      "jsonrpc": "2.0",
      "id": 1
    }
  )";

  fees = ParseGetSolanaPrioritizationFees(ParseJson(invalid_json));
  EXPECT_FALSE(fees.has_value());

  // Testing JSON with an empty 'result' array
  std::string empty_result_json = R"({
    "jsonrpc": "2.0",
    "result": [],
    "id": 1
  })";

  fees = ParseGetSolanaPrioritizationFees(ParseJson(empty_result_json));
  EXPECT_TRUE(fees.has_value());
  EXPECT_TRUE(fees->empty());

  // Testing JSON with wrong data types for 'slot' and 'prioritizationFee'
  std::string wrong_types_json = R"({
    "jsonrpc": "2.0",
    "result": [
      {
        "slot": 348125,
        "prioritizationFee": 1000
      }
    ],
    "id": 1
  })";

  fees = ParseGetSolanaPrioritizationFees(ParseJson(wrong_types_json));
  EXPECT_FALSE(fees.has_value());
}

}  // namespace brave_wallet::solana
