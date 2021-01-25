/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/translate_ipfs_uri.h"

#include <memory>
#include <vector>

#include "base/test/bind.h"
#include "brave/browser/ipfs/ipfs_service_factory.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/common/content_client.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace ipfs {

class TranslateIPFSURITest : public testing::Test {
 protected:
  void SetUp() override {
    public_gateway_ = GURL(kDefaultIPFSGateway);
    local_gateway_ = GURL("http://localhost:48080");
  }

  const GURL& local_gateway() { return local_gateway_; }
  const GURL& public_gateway() { return public_gateway_; }

 private:
  GURL local_gateway_;
  GURL public_gateway_;
};

TEST_F(TranslateIPFSURITest, TranslateIPFSURINotIPFSScheme) {
  GURL url("http://a.com/ipfs/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_FALSE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPFSScheme) {
  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
  EXPECT_EQ(new_url, GURL("https://dweb.link/ipfs/"
                          "QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPNSScheme) {
  GURL url("ipns://QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
  EXPECT_EQ(new_url, GURL("https://dweb.link/ipns/"
                          "QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd"));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPFSSchemeLocal) {
  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), false));
  EXPECT_EQ(new_url, GURL("http://localhost:48080/ipfs/"
                          "QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPNSSchemeLocal) {
  GURL url("ipns://QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), false));
  EXPECT_EQ(new_url, GURL("http://localhost:48080/ipns/"
                          "QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd"));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPFSSchemeWithPath) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
  EXPECT_EQ(new_url,
            GURL("https://dweb.link/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
                 "/wiki/Vincent_van_Gogh.html"));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPFSSchemeWithPathAndHash) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html#Emerging_artist");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
  EXPECT_EQ(new_url,
            GURL("https://dweb.link/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
                 "/wiki/Vincent_van_Gogh.html#Emerging_artist"));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPFSSchemeLocalWithPathAndHash) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html#Emerging_artist");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), false));
  EXPECT_EQ(new_url,
            GURL("http://localhost:48080/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
                 "/wiki/Vincent_van_Gogh.html#Emerging_artist"));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPFSSchemeWithPathAndQuery) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
  EXPECT_EQ(new_url,
            GURL("https://dweb.link/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
                 "/wiki/Vincent_van_Gogh.html?test=true"));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPFSSchemeLocalWithPathAndQuery) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), false));
  EXPECT_EQ(new_url,
            GURL("http://localhost:48080/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
                 "/wiki/Vincent_van_Gogh.html?test=true"));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPFSSchemeWithPathQueryHash) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true#test");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), false));
  EXPECT_EQ(new_url,
            GURL("https://dweb.link/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
                 "/wiki/Vincent_van_Gogh.html?test=true#test"));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPFSSchemeLocalWithPathQueryHash) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true#test");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), false));
  EXPECT_EQ(new_url,
            GURL("http://localhost:48080/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
                 "/wiki/Vincent_van_Gogh.html?test=true#test"));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURINotIPFSSchemeSubdomain) {
  GURL url(
      "http://a.com/ipfs/bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsgg"
      "enkbw6slwk4");
  GURL new_url;
  ASSERT_FALSE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), true));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPFSSchemeSubdomain) {
  GURL url(
      "ipfs://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("https://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkb"
                 "w6slwk4.ipfs.dweb.link/"));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPNSSchemeSubdomain) {
  GURL url(
      "ipns://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("https://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkb"
                 "w6slwk4.ipns.dweb.link/"));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPFSSchemeLocalSubdomain) {
  GURL url(
      "ipfs://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("http://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw"
                 "6slwk4.ipfs.localhost:48080/"));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPNSSchemeLocalSubdomain) {
  GURL url(
      "ipns://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), true));
  EXPECT_EQ(new_url, GURL("http://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanoko"
                          "nsggenkbw6slwk4.ipns.localhost:48080/"));
}

TEST_F(TranslateIPFSURITest, TranslateIPFSURIIPFSSchemeWithPathSubdomain) {
  GURL url(
      "ipfs://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkbw6slwk4"
      "/wiki/Vincent_van_Gogh.html");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("https://bafybeiffndsajwhk3lwjewwdxqntmjm4b5wxaaanokonsggenkb"
                 "w6slwk4.ipfs.dweb.link/wiki/Vincent_van_Gogh.html"));
}

TEST_F(TranslateIPFSURITest,
       TranslateIPFSURIIPFSSchemeWithPathAndHashSubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html#Emerging_artist");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), true));
  EXPECT_EQ(
      new_url,
      GURL(
          "https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3ev"
          "fyavhwq.ipfs.dweb.link/wiki/Vincent_van_Gogh.html#Emerging_artist"));
}

TEST_F(TranslateIPFSURITest,
       TranslateIPFSURIIPFSSchemeLocalWithPathAndHashSubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html#Emerging_artist");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("http://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evf"
                 "yavhwq.ipfs.localhost:48080/wiki/Vincent_van_Gogh.html"
                 "#Emerging_artist"));
}

TEST_F(TranslateIPFSURITest,
       TranslateIPFSURIIPFSSchemeWithPathAndQuerySubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evf"
                 "yavhwq.ipfs.dweb.link/wiki/Vincent_van_Gogh.html?test=true"));
}

TEST_F(TranslateIPFSURITest,
       TranslateIPFSURIIPFSSchemeLocalWithPathAndQuerySubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("http://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evf"
                 "yavhwq.ipfs.localhost:48080/wiki/Vincent_van_Gogh.html"
                 "?test=true"));
}

TEST_F(TranslateIPFSURITest,
       TranslateIPFSURIIPFSSchemeWithPathQueryHashSubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true#test");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3ev"
                 "fyavhwq.ipfs.dweb.link/wiki/Vincent_van_Gogh.html?test=true"
                 "#test"));
}

TEST_F(TranslateIPFSURITest,
       TranslateIPFSURIIPFSSchemeLocalWithPathQueryHashSubdomain) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html?test=true#test");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway(), true));
  EXPECT_EQ(new_url,
            GURL("http://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evf"
                 "yavhwq.ipfs.localhost:48080/wiki/Vincent_van_Gogh.html"
                 "?test=true#test"));
}

}  // namespace ipfs
