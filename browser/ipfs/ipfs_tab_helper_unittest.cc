/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/channel_info.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/version_info/channel.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/web_contents_tester.h"
#include "net/http/http_response_headers.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ipfs {

class IpfsTabHelperUnitTest : public testing::Test {
 public:
  IpfsTabHelperUnitTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()) {}
  ~IpfsTabHelperUnitTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(profile_manager_.SetUp());
    profile_ = profile_manager_.CreateTestingProfile("TestProfile");
    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile(), nullptr);
    ASSERT_TRUE(web_contents_.get());
    ASSERT_TRUE(
        ipfs::IPFSTabHelper::MaybeCreateForWebContents(web_contents_.get()));
    profile_->GetPrefs()->SetInteger(
        kIPFSResolveMethod,
        static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));
  }

  TestingProfile* profile() { return profile_; }

  ipfs::IPFSTabHelper* ipfs_tab_helper() {
    return ipfs::IPFSTabHelper::FromWebContents(web_contents_.get());
  }

  content::WebContents* web_contents() { return web_contents_.get(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  TestingProfileManager profile_manager_;
  TestingProfile* profile_;
  std::unique_ptr<content::WebContents> web_contents_;
};

TEST_F(IpfsTabHelperUnitTest, CanResolveURLTest) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);
  ASSERT_FALSE(helper->CanResolveURL(GURL("ipfs://balblabal")));
  ASSERT_FALSE(helper->CanResolveURL(GURL("file://aa")));
  ASSERT_TRUE(helper->CanResolveURL(GURL("http://a.com")));
  ASSERT_TRUE(helper->CanResolveURL(GURL("https://a.com")));

  GURL api_server = ipfs::GetAPIServer(chrome::GetChannel());
  ASSERT_FALSE(helper->CanResolveURL(api_server));
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL));

  ASSERT_TRUE(helper->CanResolveURL(GURL("https://bafyb.ipfs.dweb.link/")));
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));
  ASSERT_FALSE(helper->CanResolveURL(GURL("https://bafyb.ipfs.dweb.link/")));
}

TEST_F(IpfsTabHelperUnitTest, URLResolvingTest) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  GURL test_url("ipns://brantly.eth/page?query#ref");
  helper->SetPageURLForTesting(test_url);
  helper->IPFSLinkResolved(GURL("ipns://brantly.eth/"));
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), test_url.spec());

  test_url = GURL("ipns://brantly.eth/");
  helper->SetPageURLForTesting(test_url);
  helper->IPFSLinkResolved(GURL("ipns://brantly.eth/"));
  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), test_url.spec());

  test_url = GURL("https://ipfs.io/ipfs/bafy/wiki/empty.html?query#ref");
  helper->SetPageURLForTesting(test_url);
  helper->IPFSLinkResolved(GURL("ipfs://bafy"));

  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(),
            "ipfs://bafy/wiki/empty.html?query#ref");

  test_url = GURL("https://ipfs.io/ipfs/bafy");
  helper->SetPageURLForTesting(test_url);
  helper->IPFSLinkResolved(GURL("ipfs://bafy"));

  EXPECT_EQ(helper->GetIPFSResolvedURL().spec(), "ipfs://bafy");
}

TEST_F(IpfsTabHelperUnitTest, GatewayResolving) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  GURL api_server = GetAPIServer(chrome::GetChannel());
  helper->SetPageURLForTesting(api_server);
  helper->IPFSLinkResolved(GURL());
  ASSERT_FALSE(helper->GetIPFSResolvedURL().is_valid());

  scoped_refptr<net::HttpResponseHeaders> response_headers(
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 " +
                                                     std::to_string(200)));

  response_headers->AddHeader("x-ipfs-path", "/ipfs/bafy");
  helper->MaybeShowDNSLinkButton(response_headers.get());
  ASSERT_FALSE(helper->ipfs_resolved_url_.is_valid());

  GURL test_url("ipns://brantly.eth/");
  helper->SetPageURLForTesting(api_server);
  helper->IPFSLinkResolved(test_url);
  helper->MaybeShowDNSLinkButton(response_headers.get());
  ASSERT_FALSE(helper->ipfs_resolved_url_.is_valid());

  helper->SetPageURLForTesting(api_server);
  helper->IPFSLinkResolved(test_url);
  helper->UpdateDnsLinkButtonState();
  ASSERT_FALSE(helper->ipfs_resolved_url_.is_valid());

  helper->SetPageURLForTesting(api_server);
  helper->IPFSLinkResolved(GURL());
  helper->MaybeShowDNSLinkButton(response_headers.get());
  ASSERT_FALSE(helper->ipfs_resolved_url_.is_valid());
}

}  // namespace ipfs
