/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "brave/components/brave_wallet/browser/fil_requests.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace {
void CompareJSONs(const std::string& request_string,
                  const std::string& expected_request) {
  auto request_json = base::JSONReader::Read(request_string);
  ASSERT_TRUE(request_json);
  auto expected_request_json = base::JSONReader::Read(expected_request);
  ASSERT_TRUE(expected_request_json);
  EXPECT_EQ(*request_json, *expected_request_json);
}
}  // namespace

TEST(FilRequestUnitTest, getBalance) {
  CompareJSONs(fil::getBalance("t1jdlfl73voaiblrvn2yfivvn5ifucwwv5f26nfza"),
               R"({
                 "id": 1,
                 "jsonrpc": "2.0",
                 "method": "Filecoin.WalletBalance",
                 "params": [
                   "t1jdlfl73voaiblrvn2yfivvn5ifucwwv5f26nfza"
                 ]
               })");
}

TEST(FilRequestUnitTest, getTransactionCount) {
  CompareJSONs(
      fil::getTransactionCount("t1jdlfl73voaiblrvn2yfivvn5ifucwwv5f26nfza"),
      R"({
        "id": 1,
        "jsonrpc": "2.0",
        "method": "Filecoin.MpoolGetNonce",
        "params":[
          "t1jdlfl73voaiblrvn2yfivvn5ifucwwv5f26nfza"
        ]
      })");
}

TEST(FilRequestUnitTest, estimateGas) {
  CompareJSONs(fil::getEstimateGas("from_address", "to_address", "gas_premium",
                                   "gas_fee_cap", INT64_MAX, UINT64_MAX,
                                   "max_fee", "value"),
               R"({
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
              })");
}

}  // namespace brave_wallet
