/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/fil_requests.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(FilRequestUnitTest, fil_getBalance) {
  ASSERT_EQ(
      fil_getBalance("t1jdlfl73voaiblrvn2yfivvn5ifucwwv5f26nfza"),
      R"({"id":1,"jsonrpc":"2.0","method":"Filecoin.WalletBalance","params":["t1jdlfl73voaiblrvn2yfivvn5ifucwwv5f26nfza"]})");  // NOLINT
}

}  // namespace brave_wallet
