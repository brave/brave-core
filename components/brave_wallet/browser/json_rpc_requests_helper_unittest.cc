/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/test/gtest_util.h"
#include "base/test/values_test_util.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::ParseJson;

namespace brave_wallet {

TEST(JsonRpcRequestsHelperUnitTest, EncodeAnkrGetAccountBalanceParams) {
  auto encoded_params =
      EncodeAnkrGetAccountBalanceParams("mockAddress", {"eth", "bsc"});

  std::string expected_params(R"(
    {
      "id": 1,
      "jsonrpc": "2.0",
      "method": "ankr_getAccountBalance",
      "params": {
        "nativeFirst": true,
        "walletAddress": "mockAddress",
        "blockchains": [
          "eth",
          "bsc"
        ]
      }
    })");

  // OK: non-empty blockchains
  EXPECT_EQ(encoded_params, GetJSON(ParseJson(expected_params)));

  // OK: empty blockchains
  encoded_params = EncodeAnkrGetAccountBalanceParams("mockAddress", {});
  expected_params = R"(
    {
      "id": 1,
      "jsonrpc": "2.0",
      "method": "ankr_getAccountBalance",
      "params": {
        "nativeFirst": true,
        "walletAddress": "mockAddress",
        "blockchains": []
      }
    })";
  EXPECT_EQ(encoded_params, GetJSON(ParseJson(expected_params)));
}
}  // namespace brave_wallet
