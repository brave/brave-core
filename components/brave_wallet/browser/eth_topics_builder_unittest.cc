/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_topics_builder.h"

#include "brave/components/brave_wallet/common/hex_utils.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(EthGetLogsTopicBuilderTest, MakeAssetDiscoveryTopics) {
  // Invalid address
  base::Value::List topics;
  EXPECT_FALSE(MakeAssetDiscoveryTopics({"invalid address"}, &topics));

  // Valid
  topics.clear();
  ASSERT_TRUE(MakeAssetDiscoveryTopics(
      {"0x16e4476c8fDDc552e3b1C4b8b56261d85977fE52"}, &topics));
  EXPECT_EQ(topics[0], base::Value("0xddf252ad1be2c89b69c2b068fc378daa952ba7f16"
                                   "3c4a11628f55a4df523b3ef"));
  EXPECT_EQ(topics[1], base::Value());
  base::Value::List to_address_topic;
  to_address_topic.Append(base::Value(
      "0x00000000000000000000000016e4476c8fDDc552e3b1C4b8b56261d85977fE52"));
  EXPECT_EQ(topics[2], to_address_topic);
}

}  // namespace brave_wallet
