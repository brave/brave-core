/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/brave_wallet/browser/solana_response_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace solana {

TEST(SolanaResponseParserUnitTest, ParseSolanaGetBalance) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "{\"context\":{\"slot\":106921266},\"value\":513234116063}}";

  uint64_t balance = 0;
  EXPECT_TRUE(ParseGetBalance(json, &balance));
  EXPECT_EQ(balance, 513234116063ULL);

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "{\"context\":{\"slot\":1069},\"value\":0}}";
  EXPECT_TRUE(ParseGetBalance(json, &balance));
  EXPECT_EQ(balance, 0ULL);

  // value should be uint64
  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "{\"context\":{\"slot\":1069},\"value\":\"0\"}}";
  EXPECT_FALSE(ParseGetBalance(json, &balance));

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "{\"context\":{\"slot\":1069},\"value\":513234116063.33}}";
  EXPECT_FALSE(ParseGetBalance(json, &balance));

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "{\"context\":{\"slot\":1069},\"value\":63.33}}";
  EXPECT_FALSE(ParseGetBalance(json, &balance));
}

TEST(JsonRpcResponseParserUnitTest, ParseGetTokenAccountBalance) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "{\"context\":{\"slot\":1069},\"value\":{\"amount\":\"9864\", "
      "\"decimals\":2, \"uiAmount\":98.64, \"uiAmountString\":\"98.64\"}}}";

  std::string amount, ui_amount_string;
  uint8_t decimals = 0;
  EXPECT_TRUE(
      ParseGetTokenAccountBalance(json, &amount, &decimals, &ui_amount_string));
  EXPECT_EQ(amount, "9864");
  EXPECT_EQ(decimals, 2u);
  EXPECT_EQ(ui_amount_string, "98.64");

  // decimals should be uint8
  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "{\"context\":{\"slot\":1069},\"value\":{\"amount\":\"9864\", "
      "\"decimals\":256, \"uiAmount\":98.64, \"uiAmountString\":\"98.64\"}}}";
  EXPECT_FALSE(
      ParseGetTokenAccountBalance(json, &amount, &decimals, &ui_amount_string));

  json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "{\"context\":{\"slot\":1069},\"value\":{\"amount\":\"9864\", "
      "\"decimals\":-1, \"uiAmount\":98.64, \"uiAmountString\":\"98.64\"}}}";
  EXPECT_FALSE(
      ParseGetTokenAccountBalance(json, &amount, &decimals, &ui_amount_string));
}

TEST(SolanaResponseParserUnitTest, ParseSendTransaction) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "\"2id3YC2jK9G5Wo2phDx4gJVAew8DcY5NAojnVuao8rkxwPYPe8cSwE5GzhEgJA2y8fVjDE"
      "o6iR6ykBvDxrTQrtpb\"}";
  std::string tx_id;
  EXPECT_TRUE(ParseSendTransaction(json, &tx_id));
  EXPECT_EQ(tx_id,
            "2id3YC2jK9G5Wo2phDx4gJVAew8DcY5NAojnVuao8rkxwPYPe8cSwE5GzhEgJA2y8f"
            "VjDEo6iR6ykBvDxrTQrtpb");
}

TEST(SolanaResponseParserUnitTest, ParseGetLatestBlockhash) {
  std::string json =
      "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":"
      "{\"context\":{\"slot\":1069},\"value\":{\"blockhash\":"
      "\"EkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N\", "
      "\"lastValidBlockHeight\":3090}}}";
  std::string hash;
  EXPECT_TRUE(ParseGetLatestBlockhash(json, &hash));
  EXPECT_EQ(hash, "EkSnNWid2cvwEVnVx9aBqawnmiCNiDgp3gUdkDPTKN1N");
}

}  // namespace solana

}  // namespace brave_wallet
