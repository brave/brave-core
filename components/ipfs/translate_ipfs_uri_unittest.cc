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

class IPFSBraveContentBrowserClientTest : public testing::Test {
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

TEST_F(IPFSBraveContentBrowserClientTest, TranslateIPFSURINotIPFSScheme) {
  GURL url("http://a.com/ipfs/QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_FALSE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway()));
}

TEST_F(IPFSBraveContentBrowserClientTest, TranslateIPFSURIIPFSScheme) {
  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway()));
  EXPECT_EQ(new_url, GURL("https://dweb.link/ipfs/"
                          "QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"));
}

TEST_F(IPFSBraveContentBrowserClientTest, TranslateIPFSURIIPNSScheme) {
  GURL url("ipns://QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway()));
  EXPECT_EQ(new_url, GURL("https://dweb.link/ipns/"
                          "QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd"));
}

TEST_F(IPFSBraveContentBrowserClientTest, TranslateIPFSURIIPFSSchemeLocal) {
  GURL url("ipfs://QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway()));
  EXPECT_EQ(new_url, GURL("http://localhost:48080/ipfs/"
                          "QmfM2r8seH2GiRaC4esTjeraXEachRt8ZsSeGaWTPLyMoG"));
}

TEST_F(IPFSBraveContentBrowserClientTest, TranslateIPFSURIIPNSSchemeLocal) {
  GURL url("ipns://QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, local_gateway()));
  EXPECT_EQ(new_url, GURL("http://localhost:48080/ipns/"
                          "QmSrPmbaUKA3ZodhzPWZnpFgcPMFWF4QsxXbkWfEptTBJd"));
}

TEST_F(IPFSBraveContentBrowserClientTest, TranslateIPFSURIIPFSSchemeWithPath) {
  GURL url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      "/wiki/Vincent_van_Gogh.html");
  GURL new_url;
  ASSERT_TRUE(ipfs::TranslateIPFSURI(url, &new_url, public_gateway()));
  EXPECT_EQ(new_url,
            GURL("https://dweb.link/ipfs/"
                 "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
                 "/wiki/Vincent_van_Gogh.html"));
}

}  // namespace ipfs
