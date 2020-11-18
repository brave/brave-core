/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipfs_utils.h"

#include <vector>

#include "brave/components/ipfs/ipfs_gateway.h"
#include "chrome/common/channel_info.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

typedef testing::Test IpfsUtilsUnitTest;

TEST_F(IpfsUtilsUnitTest, HasIPFSPath) {
  std::vector<GURL> ipfs_urls(
      {GURL("http://localhost:48080/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html"),
       GURL("http://localhost:48080/ipns/tr.wikipedia-on-ipfs.org/wiki/"
            "Anasayfa.html")});

  for (auto url : ipfs_urls) {
    EXPECT_TRUE(ipfs::HasIPFSPath(url)) << url;
  }
}

TEST_F(IpfsUtilsUnitTest, IsDefaultGatewayURL) {
  std::vector<GURL> gateway_urls(
      {GURL("https://dweb.link/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html"),
       GURL("https://"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq."
            "ipfs.dweb.link/wiki/Vincent_van_Gogh.html"),
       GURL("https://dweb.link/ipns/tr.wikipedia-on-ipfs.org/wiki/"
            "Anasayfa.html")});

  std::vector<GURL> ipfs_urls(
      {GURL("http://localhost:48080/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html"),
       GURL("http://localhost:48080/ipns/tr.wikipedia-on-ipfs.org/wiki/"
            "Anasayfa.html"),
       GURL("ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
            "/wiki/Vincent_van_Gogh.html")});

  for (auto url : gateway_urls) {
    EXPECT_TRUE(ipfs::IsDefaultGatewayURL(url)) << url;
  }

  for (auto url : ipfs_urls) {
    EXPECT_FALSE(ipfs::IsDefaultGatewayURL(url)) << url;
  }
}

TEST_F(IpfsUtilsUnitTest, IsLocalGatewayURL) {
  std::vector<GURL> local_gateway_urls(
      {GURL("http://localhost:48080/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html"),
       GURL(
           "http://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq."
           "ipfs.localhost:48080//wiki/Vincent_van_Gogh.html"),
       GURL("http://127.0.0.1:48080/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html")});

  std::vector<GURL> non_local_gateway_urls(
      {GURL("https://dweb.link/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html"),
       GURL("ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
            "/wiki/Vincent_van_Gogh.html"),
       GURL("http://github.com/ipfs/go-ipfs")});

  for (auto url : local_gateway_urls) {
    EXPECT_TRUE(ipfs::IsLocalGatewayURL(url)) << url;
  }

  for (auto url : non_local_gateway_urls) {
    EXPECT_FALSE(ipfs::IsLocalGatewayURL(url)) << url;
  }
}

TEST_F(IpfsUtilsUnitTest, ToPublicGatewayURL) {
  std::vector<GURL> ipfs_urls(
      {GURL("http://localhost:48080/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html"),
       GURL("http://127.0.0.1:48080/ipfs/"
            "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
            "Vincent_van_Gogh.html"),
       GURL("ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
            "/wiki/Vincent_van_Gogh.html")});

  GURL expected_new_url = GURL(
      "https://dweb.link/ipfs/"
      "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
      "Vincent_van_Gogh.html");

  for (auto url : ipfs_urls) {
    GURL new_url = ipfs::ToPublicGatewayURL(url);
    EXPECT_EQ(new_url, expected_new_url) << url;
  }
}

TEST_F(IpfsUtilsUnitTest, GetIPFSGatewayURL) {
  EXPECT_EQ(
      ipfs::GetIPFSGatewayURL(
          "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq", "",
          ipfs::GetDefaultIPFSGateway()),
      GURL(
          "https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq."
          "ipfs.dweb.link"));
  EXPECT_EQ(
      ipfs::GetIPFSGatewayURL(
          "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq", "",
          ipfs::GetDefaultIPFSGateway()),
      GURL(
          "https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq."
          "ipfs.dweb.link"));
}

TEST_F(IpfsUtilsUnitTest, GetIPFSGatewayURLLocal) {
  EXPECT_EQ(
      ipfs::GetIPFSGatewayURL(
          "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq", "",
          ipfs::GetDefaultIPFSLocalGateway(chrome::GetChannel())),
      GURL("http://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq."
           "ipfs.localhost:48080"));
  EXPECT_EQ(
      ipfs::GetIPFSGatewayURL(
          "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq", "",
          ipfs::GetDefaultIPFSLocalGateway(chrome::GetChannel())),
      GURL("http://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq."
           "ipfs.localhost:48080"));
}
