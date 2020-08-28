/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/common/ipfs_utils.h"

#include <vector>

#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

typedef testing::Test IpfsUtilsUnitTest;

TEST_F(IpfsUtilsUnitTest, IsIPFSURL) {
  std::vector<GURL> ipfs_urls({
      GURL("http://localhost:8080/ipfs/bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/Vincent_van_Gogh.html"),  // NOLINT
      GURL("http://localhost:8080/ipns/tr.wikipedia-on-ipfs.org/wiki/Anasayfa.html")  // NOLINT
  });

  for (auto url : ipfs_urls) {
    EXPECT_TRUE(ipfs::IpfsUtils::IsIPFSURL(url)) << url;
  }
}
