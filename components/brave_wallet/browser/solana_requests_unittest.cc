/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_requests.h"

#include <optional>

#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/test/gtest_util.h"
#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

constexpr char kBlockhash[] = "J7rBdM6AecPDEZp8aPq5iPSNKVkU5Q76F3oAV4eW5wsW";

}

namespace brave_wallet::solana {

TEST(SolanaRequestsUnitTest, getBalance) {
  ASSERT_EQ(
      getBalance("key"),
      R"({"id":1,"jsonrpc":"2.0","method":"getBalance","params":["key"]})");
}

TEST(SolanaRequestsUnitTest, getTokenAccountBalance) {
  ASSERT_EQ(
      getTokenAccountBalance("key"),
      R"({"id":1,"jsonrpc":"2.0","method":"getTokenAccountBalance","params":["key"]})");
}

TEST(SolanaRequestsUnitTest, sendTransaction) {
  ASSERT_EQ(
      sendTransaction("signed_tx", std::nullopt),
      R"({"id":1,"jsonrpc":"2.0","method":"sendTransaction","params":["signed_tx",{"encoding":"base64"}]})");

  std::string expected_json_string = R"(
      {
        "id":1,"jsonrpc":"2.0","method":"sendTransaction",
        "params":["signed_tx", {
          "encoding": "base64",
          "maxRetries": 18446744073709551615,
          "preflightCommitment": "confirmed",
          "skipPreflight": true
        }]
      }
  )";
  auto expected_json = base::JSONReader::Read(expected_json_string);
  ASSERT_TRUE(expected_json);
  std::string json_string = sendTransaction(
      "signed_tx",
      SolanaTransaction::SendOptions(UINT64_MAX, "confirmed", true));
  auto json = base::JSONReader::Read(json_string);
  ASSERT_TRUE(json);
  EXPECT_EQ(*json, *expected_json);
}

TEST(SolanaRequestsUnitTest, getLatestBlockhash) {
  ASSERT_EQ(
      getLatestBlockhash(),
      R"({"id":1,"jsonrpc":"2.0","method":"getLatestBlockhash","params":[]})");
}

TEST(SolanaRequestsUnitTest, getSignatureStatuses) {
  std::string expected_json_string = R"(
      {
        "id":1,"jsonrpc":"2.0","method":"getSignatureStatuses",
        "params":[
          [
            "5VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8uirBgmQpjKhoR4tjF3ZpRzrFmBV6UjKdiSZkQUW",
            "5j7s6NiJS3JAkvgkoc18WVAsiSaci2pxB2A6ueCJP4tprA2TFg9wSyTLeYouxPBJEMzJinENTkpA52YStRW5Dia7"
          ],
          {
            "searchTransactionHistory": true
          }
        ]
      })";
  auto expected_json = base::JSONReader::Read(expected_json_string);
  ASSERT_TRUE(expected_json);
  std::string json_string =
      getSignatureStatuses({"5VERv8NMvzbJMEkV8xnrLkEaWRtSz9CosKDYjCJjBRnbJLgp8u"
                            "irBgmQpjKhoR4tjF3ZpRzrFmBV6UjKdiSZkQUW",
                            "5j7s6NiJS3JAkvgkoc18WVAsiSaci2pxB2A6ueCJP4tprA2TFg"
                            "9wSyTLeYouxPBJEMzJinENTkpA52YStRW5Dia7"});
  auto json = base::JSONReader::Read(json_string);
  ASSERT_TRUE(json);
  EXPECT_EQ(*json, *expected_json);
}

TEST(SolanaRequestsUnitTest, getAccountInfo) {
  ASSERT_EQ(
      getAccountInfo("pubkey"),
      R"({"id":1,"jsonrpc":"2.0","method":"getAccountInfo","params":["pubkey",{"encoding":"base64"}]})");
}

TEST(SolanaRequestsUnitTest, getFeeForMessage) {
  ASSERT_EQ(
      getFeeForMessage("message"),
      R"({"id":1,"jsonrpc":"2.0","method":"getFeeForMessage","params":["message",{"commitment":"confirmed"}]})");
}

TEST(SolanaRequestsUnitTest, getBlockHeight) {
  ASSERT_EQ(
      getBlockHeight(),
      R"({"id":1,"jsonrpc":"2.0","method":"getBlockHeight","params":[]})");
}

TEST(SolanaRequestsUnitTest, getTokenAccountsByOwner) {
  constexpr char kExpectedJsonStringFormat[] = R"(
    {
      "id":1,
      "jsonrpc":"2.0",
      "method":"getTokenAccountsByOwner",
      "params":[
        "pubkey",
        {
          "programId":"program"
        },
        {
          "encoding":"%s"
        }
      ]
    }
  )";
  ASSERT_EQ(base::test::ParseJsonDict(
                getTokenAccountsByOwner("pubkey", "base64", "program")),
            base::test::ParseJsonDict(
                base::StringPrintf(kExpectedJsonStringFormat, "base64")));

  ASSERT_EQ(base::test::ParseJsonDict(
                getTokenAccountsByOwner("pubkey", "base58", "program")),
            base::test::ParseJsonDict(
                base::StringPrintf(kExpectedJsonStringFormat, "base58")));

  ASSERT_EQ(base::test::ParseJsonDict(
                getTokenAccountsByOwner("pubkey", "jsonParsed", "program")),
            base::test::ParseJsonDict(
                base::StringPrintf(kExpectedJsonStringFormat, "jsonParsed")));

  EXPECT_CHECK_DEATH(
      getTokenAccountsByOwner("pubkey", "invalid encoding", "program"));
}

TEST(SolanaRequestsUnitTest, isBlockhashValid) {
  EXPECT_EQ(
      base::test::ParseJsonDict(isBlockhashValid(kBlockhash, std::nullopt)),
      base::test::ParseJsonDict(
          R"({"id": 1,
              "jsonrpc": "2.0",
              "method": "isBlockhashValid",
              "params": [
                "J7rBdM6AecPDEZp8aPq5iPSNKVkU5Q76F3oAV4eW5wsW",
                {"commitment": "processed"}
              ]})"));

  EXPECT_EQ(
      base::test::ParseJsonDict(isBlockhashValid(kBlockhash, "finalized")),
      base::test::ParseJsonDict(
          R"({"id": 1,
              "jsonrpc": "2.0",
              "method": "isBlockhashValid",
              "params": [
                "J7rBdM6AecPDEZp8aPq5iPSNKVkU5Q76F3oAV4eW5wsW",
                {"commitment": "finalized"}
              ]})"));

  EXPECT_CHECK_DEATH(isBlockhashValid(kBlockhash, "invalid_commitment"));
}

TEST(SolanaRequestsUnitTest, simulateTransaction) {
  EXPECT_EQ(base::test::ParseJsonDict(simulateTransaction("unsigned tx")),
            base::test::ParseJsonDict(
                R"({
                  "id": 1,
                  "jsonrpc": "2.0",
                  "method": "simulateTransaction",
                  "params": [
                    "unsigned tx",
                    {
                      "commitment": "confirmed",
                      "encoding": "base64"
                    }
                  ]
                })"));
}

TEST(SolanaRequestsUnitTest, getRecentPrioritizationFees) {
  EXPECT_EQ(base::test::ParseJsonDict(getRecentPrioritizationFees()),
            base::test::ParseJsonDict(
                R"({
                "id": 1,
                "jsonrpc": "2.0",
                "method": "getRecentPrioritizationFees",
                "params": []
              })"));
}

}  // namespace brave_wallet::solana
