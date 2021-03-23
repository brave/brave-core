/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_call_data_builder.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

namespace erc20 {

TEST(EthCallDataBuilderTest, BalanceOf) {
  std::string data;
  BalanceOf("0x4e02f254184E904300e0775E4b8eeCB1", &data);
  ASSERT_EQ(data,
            "0x70a08231000000000000000000000000000000004e02f254184E904300e0775E"
            "4b8eeCB1");
}

}  // namespace erc20

}  // namespace brave_wallet
