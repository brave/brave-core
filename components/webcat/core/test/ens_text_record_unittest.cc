/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/ens_resolver_task.h"

#include "brave/components/brave_wallet/common/eth_abi_utils.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EnsTextRecordTest, MakeTextRecordCallSelector) {
  auto call = MakeTextRecordCall("app.eth", "webcat");

  auto [selector, args] =
      *eth_abi::ExtractFunctionSelectorAndArgsFromCall(call);

  EXPECT_EQ(selector.size(), 4u);
  EXPECT_EQ(selector[0], kTextBytes32StringSelector[0]);
  EXPECT_EQ(selector[1], kTextBytes32StringSelector[1]);
  EXPECT_EQ(selector[2], kTextBytes32StringSelector[2]);
  EXPECT_EQ(selector[3], kTextBytes32StringSelector[3]);
}

TEST(EnsTextRecordTest, MakeTextRecordCallNamehash) {
  auto call = MakeTextRecordCall("app.eth", "webcat");

  auto [selector, args] =
      *eth_abi::ExtractFunctionSelectorAndArgsFromCall(call);

  auto domain_namehash =
      eth_abi::ExtractFixedBytesFromTuple<32>(args, 0);
  ASSERT_TRUE(domain_namehash.has_value());

  auto expected = Namehash("app.eth");
  EXPECT_TRUE(std::ranges::equal(*domain_namehash, expected));
}

TEST(EnsTextRecordTest, MakeTextRecordCallKey) {
  auto call = MakeTextRecordCall("app.eth", "webcat");

  auto [selector, args] =
      *eth_abi::ExtractFunctionSelectorAndArgsFromCall(call);

  auto key = eth_abi::ExtractStringFromTuple(args, 1);
  ASSERT_TRUE(key.has_value());
  EXPECT_EQ(*key, "webcat");
}

TEST(EnsTextRecordTest, MakeTextRecordCallCustomKey) {
  auto call = MakeTextRecordCall("example.eth", "custom-record");

  auto [selector, args] =
      *eth_abi::ExtractFunctionSelectorAndArgsFromCall(call);

  auto key = eth_abi::ExtractStringFromTuple(args, 1);
  ASSERT_TRUE(key.has_value());
  EXPECT_EQ(*key, "custom-record");
}

}  // namespace brave_wallet