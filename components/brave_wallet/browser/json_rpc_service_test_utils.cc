/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_rpc_service_test_utils.h"

#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

std::string MakeJsonRpcStringArrayResponse(
    const std::vector<std::string>& items) {
  std::string encoded_head;
  EXPECT_TRUE(PadHexEncodedParameter(Uint256ValueToHex(32), &encoded_head));
  std::string encoded_array;
  EXPECT_TRUE(EncodeStringArray(items, &encoded_array));
  encoded_array = encoded_array.substr(2);
  return base::StringPrintf(R"({"jsonrpc":"2.0", "id":1, "result":"%s"})",
                            (encoded_head + encoded_array).c_str());
}

std::string MakeJsonRpcStringResponse(const std::string& str) {
  std::string encoded_head;
  EXPECT_TRUE(PadHexEncodedParameter(Uint256ValueToHex(32), &encoded_head));
  std::string encoded_string;
  EXPECT_TRUE(EncodeString(str, &encoded_string));
  encoded_string = encoded_string.substr(2);
  return base::StringPrintf(R"({"jsonrpc":"2.0", "id":1, "result":"%s"})",
                            (encoded_head + encoded_string).c_str());
}

std::string MakeJsonRpcErrorResponse(int error,
                                     const std::string& error_message) {
  return base::StringPrintf(R"({"jsonrpc":"2.0", "id":1,)"
                            R"("error": {"code":%d, "message": "%s"})"
                            R"(})",
                            error, error_message.c_str());
}

}  // namespace brave_wallet
