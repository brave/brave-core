/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"

#include "base/test/values_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(JsonRpcRequestsHelperUnitTest, EncodeAnkrGetAccountBalancesParams) {
  auto encoded_params =
      EncodeAnkrGetAccountBalancesParams("mockAddress", {"eth", "bsc"});

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
  EXPECT_THAT(encoded_params, base::test::IsJson(expected_params));

  // OK: empty blockchains
  encoded_params = EncodeAnkrGetAccountBalancesParams("mockAddress", {});
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
  EXPECT_THAT(encoded_params, base::test::IsJson(expected_params));
}
}  // namespace brave_wallet
