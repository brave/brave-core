/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/microsoft_edge_protocol_util.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(MSEdgeProtocolTest, BasicTest) {
  EXPECT_FALSE(GetURLFromMSEdgeProtocol(L""));
  EXPECT_FALSE(GetURLFromMSEdgeProtocol(L"https://www.brave.com/"));
  EXPECT_TRUE(
      GetURLFromMSEdgeProtocol(L"microsoft-edge:https://www.brave.com/"));
  EXPECT_EQ(
      "https://www.brave.com/",
      *GetURLFromMSEdgeProtocol(L"microsoft-edge:https://www.brave.com/"));
  // Test encoded url. GetURLFromMSEdgeProtocol() will return decoded url.
  EXPECT_EQ(
      "https://www.bing.com/search?q=test",
      *GetURLFromMSEdgeProtocol(
          L"microsoft-edge:https%3A%2F%2Fwww.bing.com%2Fsearch%3Fq%3Dtest"));
  // Test cortana args.
  EXPECT_EQ(
      "https://www.bing.com/search?q=test",
      *GetURLFromMSEdgeProtocol(
          L"microsoft-edge:?launchContext1=Microsoft.Windows.Cortana_"
          L"cw5n1h2txyewy&url=https%3A%2F%2Fwww.bing.com%2Fsearch%3Fq%3Dtest"));
}
