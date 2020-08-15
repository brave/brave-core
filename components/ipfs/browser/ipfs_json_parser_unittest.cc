/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>
#include <vector>

#include "brave/components/ipfs/browser/ipfs_json_parser.h"
#include "testing/gtest/include/gtest/gtest.h"

typedef testing::Test IPFSJSONParserTest;

TEST_F(IPFSJSONParserTest, GetPeersFromJSON) {
  std::vector<std::string> peers;
  ASSERT_TRUE(IPFSJSONParser::GetPeersFromJSON(R"(
      {
        "Peers": [
          {
            "Addr": "10.8.0.206",
            "Direction": "0",
            "Latency": "",
            "Muxer": "",
            "Peer": "QmaNcj4BMFQgE884rZSMqWEcqquWuv8QALzhpvPeHZGddd"
          },
          {
            "Addr": "10.8.0.207",
            "Direction": "0",
            "Latency": "",
            "Muxer": "",
            "Peer": "QmaNcj4BMFQgE884rZSMqWEcqquWuv8QALzhpvPeHZGeee"
          }
        ]
      })", &peers));

  ASSERT_EQ(peers.size(), uint64_t(2));
  ASSERT_EQ(peers[0],
            "10.8.0.206/QmaNcj4BMFQgE884rZSMqWEcqquWuv8QALzhpvPeHZGddd");
  ASSERT_EQ(peers[1],
            "10.8.0.207/QmaNcj4BMFQgE884rZSMqWEcqquWuv8QALzhpvPeHZGeee");
}
