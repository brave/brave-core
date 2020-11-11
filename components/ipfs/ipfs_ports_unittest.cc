/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_ports.h"
#include "components/version_info/channel.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(IPFSPortsTest, GetAPIPort) {
  EXPECT_EQ("45001", ipfs::GetAPIPort(version_info::Channel::UNKNOWN));
  EXPECT_EQ("45001", ipfs::GetAPIPort(version_info::Channel::DEFAULT));
  EXPECT_EQ("45002", ipfs::GetAPIPort(version_info::Channel::CANARY));
  EXPECT_EQ("45003", ipfs::GetAPIPort(version_info::Channel::DEV));
  EXPECT_EQ("45004", ipfs::GetAPIPort(version_info::Channel::BETA));
  EXPECT_EQ("45005", ipfs::GetAPIPort(version_info::Channel::STABLE));
}

TEST(IPFSPortsTest, GetGatewayPort) {
  EXPECT_EQ("48080", ipfs::GetGatewayPort(version_info::Channel::UNKNOWN));
  EXPECT_EQ("48080", ipfs::GetGatewayPort(version_info::Channel::DEFAULT));
  EXPECT_EQ("48081", ipfs::GetGatewayPort(version_info::Channel::CANARY));
  EXPECT_EQ("48082", ipfs::GetGatewayPort(version_info::Channel::DEV));
  EXPECT_EQ("48083", ipfs::GetGatewayPort(version_info::Channel::BETA));
  EXPECT_EQ("48084", ipfs::GetGatewayPort(version_info::Channel::STABLE));
}

TEST(IPFSPortsTest, GetSwarmPort) {
  EXPECT_EQ("44001", ipfs::GetSwarmPort(version_info::Channel::UNKNOWN));
  EXPECT_EQ("44001", ipfs::GetSwarmPort(version_info::Channel::DEFAULT));
  EXPECT_EQ("44002", ipfs::GetSwarmPort(version_info::Channel::CANARY));
  EXPECT_EQ("44003", ipfs::GetSwarmPort(version_info::Channel::DEV));
  EXPECT_EQ("44004", ipfs::GetSwarmPort(version_info::Channel::BETA));
  EXPECT_EQ("44005", ipfs::GetSwarmPort(version_info::Channel::STABLE));
}
