/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/content_browser_client_helper.h"

#include <memory>
#include <vector>

#include "base/test/bind_test_util.h"
#include "base/test/scoped_feature_list.h"
#include "brave/browser/profiles/brave_profile_manager.h"
#include "brave/browser/profiles/brave_unittest_profile_manager.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/pref_names.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/prefs/browser_prefs.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/mock_navigation_handle.h"
#include "content/public/test/test_utils.h"
#include "content/public/test/web_contents_tester.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace {

const GURL& GetIPFSURI() {
  static const GURL ipfs_url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
      "Vincent_van_Gogh.html");  // NOLINT
  return ipfs_url;
}

const GURL& GetIPFSGatewayURL() {
  static const GURL ipfs_url(
      "https://dweb.link/ipfs/"
      "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
      "Vincent_van_Gogh.html");  // NOLINT
  return ipfs_url;
}

const GURL& GetIPFSLocalURL() {
  static const GURL ipfs_url(
      "http://127.0.0.1:48080/ipfs/"
      "bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
      "Vincent_van_Gogh.html");  // NOLINT
  return ipfs_url;
}

const GURL& GetIPNSURI() {
  static const GURL ipns_url(
      "ipns://tr.wikipedia-on-ipfs.org/wiki/Anasayfa.html");  // NOLINT
  return ipns_url;
}

const GURL& GetIPNSGatewayURL() {
  static const GURL ipfs_url(
      "https://dweb.link/ipns/tr.wikipedia-on-ipfs.org/wiki/Anasayfa.html");  // NOLINT
  return ipfs_url;
}

}  // namespace

using content::NavigationThrottle;

namespace ipfs {

class ContentBrowserClientHelperUnitTest : public testing::Test {
 public:
  ContentBrowserClientHelperUnitTest()
      : local_state_(TestingBrowserProcess::GetGlobal()) {}
  ~ContentBrowserClientHelperUnitTest() override = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeature(ipfs::features::kIpfsFeature);
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());

    TestingBrowserProcess::GetGlobal()->SetProfileManager(
        new BraveUnittestProfileManager(temp_dir_.GetPath()));
    ProfileManager* profile_manager = g_browser_process->profile_manager();
    ASSERT_TRUE(profile_manager);

    profile_ = profile_manager->GetProfile(
        temp_dir_.GetPath().AppendASCII(TestingProfile::kTestUserProfileDir));
    web_contents_ =
        content::WebContentsTester::CreateTestWebContents(profile_, nullptr);
  }

  void TearDown() override {
    web_contents_.reset();
    TestingBrowserProcess::GetGlobal()->SetProfileManager(nullptr);
  }

  content::WebContents* web_contents() { return web_contents_.get(); }

  base::ScopedTempDir* temp_dir() { return &temp_dir_; }

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
  base::ScopedTempDir temp_dir_;
  ScopedTestingLocalState local_state_;
  content::RenderViewHostTestEnabler test_render_host_factories_;
  std::unique_ptr<content::WebContents> web_contents_;
  Profile* profile_;
  base::test::ScopedFeatureList feature_list_;

  DISALLOW_COPY_AND_ASSIGN(ContentBrowserClientHelperUnitTest);
};

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPFSURLRewriteDisabled) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_DISABLED));
  GURL ipfs_uri(GetIPFSURI());
  ASSERT_FALSE(ContentBrowserClientHelper::HandleIPFSURLRewrite(
      &ipfs_uri, browser_context()));
}

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPFSURLRewriteAsk) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_ASK));
  GURL ipfs_uri(GetIPFSURI());
  ASSERT_FALSE(ContentBrowserClientHelper::HandleIPFSURLRewrite(
      &ipfs_uri, browser_context()));
}

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPFSURLRewriteGateway) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));
  GURL ipfs_uri(GetIPFSURI());
  ASSERT_FALSE(ContentBrowserClientHelper::HandleIPFSURLRewrite(
      &ipfs_uri, browser_context()));
}

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPFSURLRewriteLocal) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));
  GURL ipfs_uri(GetIPFSURI());
  ASSERT_TRUE(ContentBrowserClientHelper::HandleIPFSURLRewrite(
      &ipfs_uri, browser_context()));
}

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPNSURLRewriteLocal) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));
  GURL ipns_uri(GetIPNSURI());
  ASSERT_TRUE(ContentBrowserClientHelper::HandleIPFSURLRewrite(
      &ipns_uri, browser_context()));
}

TEST_F(ContentBrowserClientHelperUnitTest, ShouldNavigateIPFSURIDisabled) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_DISABLED));
  GURL new_url;
  ASSERT_FALSE(ContentBrowserClientHelper::ShouldNavigateIPFSURI(
      GetIPFSURI(), &new_url, browser_context()));
}

TEST_F(ContentBrowserClientHelperUnitTest,
       ShouldNavigateIPFSURIGatewayIPFSURI) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));
  GURL new_url;
  ASSERT_TRUE(ContentBrowserClientHelper::ShouldNavigateIPFSURI(
      GetIPFSURI(), &new_url, browser_context()));
  ASSERT_EQ(new_url, GetIPFSGatewayURL());
}

TEST_F(ContentBrowserClientHelperUnitTest,
       ShouldNavigateIPFSURIGatewayIPFSHTTPURI) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));
  GURL new_url;
  ASSERT_TRUE(ContentBrowserClientHelper::ShouldNavigateIPFSURI(
      GetIPFSGatewayURL(), &new_url, browser_context()));
  ASSERT_EQ(new_url, GetIPFSGatewayURL());
}

TEST_F(ContentBrowserClientHelperUnitTest, ShouldNavigateIPFSURILocalIPFSURI) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));
  GURL new_url;
  ASSERT_TRUE(ContentBrowserClientHelper::ShouldNavigateIPFSURI(
      GetIPFSURI(), &new_url, browser_context()));
  ASSERT_EQ(new_url, GetIPFSLocalURL());
}

TEST_F(ContentBrowserClientHelperUnitTest,
       ShouldNavigateIPFSURILocalIPFSHTTPURI) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));
  GURL new_url;
  ASSERT_TRUE(ContentBrowserClientHelper::ShouldNavigateIPFSURI(
      GetIPFSLocalURL(), &new_url, browser_context()));
  ASSERT_EQ(new_url, GetIPFSLocalURL());
}

TEST_F(ContentBrowserClientHelperUnitTest,
       ShouldNavigateIPFSURIGatewayIPNSURI) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));
  GURL new_url;
  ASSERT_TRUE(ContentBrowserClientHelper::ShouldNavigateIPFSURI(
      GetIPNSURI(), &new_url, browser_context()));
  ASSERT_EQ(new_url, GetIPNSGatewayURL());
}

}  // namespace ipfs
