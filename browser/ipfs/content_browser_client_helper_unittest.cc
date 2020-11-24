/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/content_browser_client_helper.h"

#include <memory>
#include <vector>

#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_gateway.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/common/channel_info.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/test_utils.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

constexpr char kTestProfileName[] = "TestProfile";

const GURL& GetIPFSURI() {
  static const GURL ipfs_url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
      "Vincent_van_Gogh.html");  // NOLINT
  return ipfs_url;
}

const GURL& GetIPNSURI() {
  static const GURL ipns_url(
      "ipns://tr.wikipedia-on-ipfs.org/wiki/Anasayfa.html");  // NOLINT
  return ipns_url;
}

}  // namespace

using content::NavigationThrottle;

namespace ipfs {

class ContentBrowserClientHelperUnitTest : public testing::Test {
 public:
  ContentBrowserClientHelperUnitTest() = default;
  ~ContentBrowserClientHelperUnitTest() override = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeature(ipfs::features::kIpfsFeature);

    TestingBrowserProcess* browser_process = TestingBrowserProcess::GetGlobal();
    profile_manager_.reset(new TestingProfileManager(browser_process));
    ASSERT_TRUE(profile_manager_->SetUp());

    profile_ = profile_manager_->CreateTestingProfile(kTestProfileName);

    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile_, nullptr);
  }

  void TearDown() override {
    web_contents_.reset();
    profile_ = nullptr;
    profile_manager_->DeleteTestingProfile(kTestProfileName);
  }

  content::WebContents* web_contents() { return web_contents_.get(); }

  // Helper that creates simple test guest profile.
  std::unique_ptr<TestingProfile> CreateGuestProfile() {
    TestingProfile::Builder profile_builder;
    profile_builder.SetGuestSession();
    return profile_builder.Build();
  }

  Profile* profile() { return profile_; }

  content::BrowserContext* browser_context() {
    return web_contents()->GetBrowserContext();
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler test_render_host_factories_;
  std::unique_ptr<content::WebContents> web_contents_;
  Profile* profile_;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  base::test::ScopedFeatureList feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ContentBrowserClientHelperUnitTest);
};

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPFSURLRewriteDisabled) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_DISABLED));
  GURL ipfs_uri(GetIPFSURI());
  ASSERT_FALSE(HandleIPFSURLRewrite(&ipfs_uri, browser_context()));
}

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPFSURLRewriteAsk) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_ASK));
  GURL ipfs_uri(GetIPFSURI());
  ASSERT_FALSE(HandleIPFSURLRewrite(&ipfs_uri, browser_context()));
}

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPFSURLRewriteGateway) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));
  GURL ipfs_uri(GetIPFSURI());
  ASSERT_FALSE(HandleIPFSURLRewrite(&ipfs_uri, browser_context()));
}

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPFSURLRewriteLocal) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));
  GURL ipfs_uri(GetIPFSURI());
  ASSERT_TRUE(HandleIPFSURLRewrite(&ipfs_uri, browser_context()));
}

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPNSURLRewriteLocal) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));
  GURL ipns_uri(GetIPNSURI());
  ASSERT_TRUE(HandleIPFSURLRewrite(&ipns_uri, browser_context()));
}

}  // namespace ipfs
