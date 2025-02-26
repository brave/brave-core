/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/fil_requests.h"

#include <memory>
#include <utility>
#include <vector>

#include "base/test/values_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(FilRequestUnitTest, getBalance) {
  EXPECT_EQ(base::test::ParseJson(
                fil::getBalance("t1jdlfl73voaiblrvn2yfivvn5ifucwwv5f26nfza")),
            base::test::ParseJson(R"({
                 "id": 1,
                 "jsonrpc": "2.0",
                 "method": "Filecoin.WalletBalance",
                 "params": [
                   "t1jdlfl73voaiblrvn2yfivvn5ifucwwv5f26nfza"
                 ]
               })"));
}

TEST(FilRequestUnitTest, getTransactionCount) {
  EXPECT_EQ(base::test::ParseJson(fil::getTransactionCount(
                "t1jdlfl73voaiblrvn2yfivvn5ifucwwv5f26nfza")),
            base::test::ParseJson(R"({
        "id": 1,
        "jsonrpc": "2.0",
        "method": "Filecoin.MpoolGetNonce",
        "params":[
          "t1jdlfl73voaiblrvn2yfivvn5ifucwwv5f26nfza"
        ]
      })"));
}

TEST(FilRequestUnitTest, estimateGas) {
  EXPECT_EQ(base::test::ParseJson(fil::getEstimateGas(
                "from_address", "to_address", "gas_premium", "gas_fee_cap",
                INT64_MAX, UINT64_MAX, "max_fee", "value")),
            base::test::ParseJson(R"({
                "id": 1,
                "jsonrpc": "2.0",
                "method": "Filecoin.GasEstimateMessageGas",
                "params": [
                    {
                        "From": "from_address",
                        "GasFeeCap": "gas_fee_cap",
                        "GasLimit": 9223372036854775807,
                        "GasPremium": "gas_premium",
                        "Method": 0,
                        "Nonce": 18446744073709551615,
                        "Params": "",
                        "To": "to_address",
                        "Value": "value",
                        "Version": 0
                    },
                    {
                        "MaxFee": "max_fee"
                    },
                    []
                ]
              })"));
}

TEST(FilRequestUnitTest, estimateGas_WhenSendToFEVM) {
  EXPECT_EQ(base::test::ParseJson(fil::getEstimateGas(
                "from_address", "t410frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y",
                "gas_premium", "gas_fee_cap", INT64_MAX, UINT64_MAX, "max_fee",
                "value")),
            base::test::ParseJson(R"({
                "id": 1,
                "jsonrpc": "2.0",
                "method": "Filecoin.GasEstimateMessageGas",
                "params": [
                    {
                        "From": "from_address",
                        "GasFeeCap": "gas_fee_cap",
                        "GasLimit": 9223372036854775807,
                        "GasPremium": "gas_premium",
                        "Method": 3844450837,
                        "Nonce": 18446744073709551615,
                        "Params": "",
                        "To": "t410frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y",
                        "Value": "value",
                        "Version": 0
                    },
                    {
                        "MaxFee": "max_fee"
                    },
                    []
                ]
              })"));
}

TEST(FilRequestUnitTest, getChainHead) {
  EXPECT_EQ(fil::getChainHead(),
            "{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"Filecoin.ChainHead\","
            "\"params\":[]}");
}
TEST(FilRequestUnitTest, getStateSearchMsgLimited) {
  EXPECT_EQ(fil::getStateSearchMsgLimited("cid", UINT64_MAX),
            "{\"id\":1,\"jsonrpc\":\"2.0\",\"method\":\"Filecoin."
            "StateSearchMsgLimited\",\"params\":[{\"/\":\"cid\"}," +
                std::to_string(UINT64_MAX) + "]}");
}

TEST(FilRequestUnitTest, getSendTransaction_WhenSendToFEVM) {
  auto send = fil::getSendTransaction(R"({
    "Message": {
        "From": "from",
        "GasFeeCap": "3",
        "GasLimit": 1,
        "GasPremium": "2",
        "Method": 3844450837,
        "Params": "",
        "Nonce": 1,
        "To": "f410frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y",
        "Value": "6",
        "Version": 0
      },
      "Signature": {
        "Type": 1,
        "Data": "signed_tx"
      }
    })");
  ASSERT_TRUE(send);
  EXPECT_EQ(base::test::ParseJson(*send), base::test::ParseJson(R"({
                "id": 1,
                "jsonrpc": "2.0",
                "method": "Filecoin.MpoolPush",
                "params": [{
                  "Message": {
                      "From": "from",
                      "GasFeeCap": "3",
                      "GasLimit": 1,
                      "GasPremium": "2",
                      "Method": 3844450837,
                      "Params": "",
                      "Nonce": 1,
                      "To": "f410frrqkhkktbxosf5cmboocdhsv42jtgw2rddjac2y",
                      "Value": "6",
                      "Version": 0
                    },
                    "Signature": {
                      "Type": 1,
                      "Data": "signed_tx"
                    }
                  }
                ]
              })"));
}

TEST(FilRequestUnitTest, getSendTransaction) {
  auto send = fil::getSendTransaction(R"({
    "Message": {
        "From": "from",
        "GasFeeCap": "3",
        "GasLimit": 1,
        "GasPremium": "2",
        "Method": 0,
        "Params": "",
        "Nonce": 1,
        "To": "to",
        "Value": "6",
        "Version": 0
      },
      "Signature": {
        "Type": 1,
        "Data": "signed_tx"
      }
    })");
  ASSERT_TRUE(send);
  EXPECT_EQ(base::test::ParseJson(*send), base::test::ParseJson(R"({
                "id": 1,
                "jsonrpc": "2.0",
                "method": "Filecoin.MpoolPush",
                "params": [{
                  "Message": {
                      "From": "from",
                      "GasFeeCap": "3",
                      "GasLimit": 1,
                      "GasPremium": "2",
                      "Method": 0,
                      "Params": "",
                      "Nonce": 1,
                      "To": "to",
                      "Value": "6",
                      "Version": 0
                    },
                    "Signature": {
                      "Type": 1,
                      "Data": "signed_tx"
                    }
                  }
                ]
              })"));
  // broken json
  EXPECT_FALSE(fil::getSendTransaction("broken"));
  // empty json
  EXPECT_FALSE(fil::getSendTransaction(""));
}

}  // namespace brave_wallet
