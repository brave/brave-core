/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/content_browser_client_helper.h"

#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "base/test/scoped_feature_list.h"
#include "brave/common/webui_url_constants.h"
#include "brave/components/decentralized_dns/buildflags/buildflags.h"
#include "brave/components/ipfs/features.h"
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_ports.h"
#include "brave/components/ipfs/ipfs_utils.h"
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

#if BUILDFLAG(DECENTRALIZED_DNS_ENABLED)
#include "brave/components/decentralized_dns/constants.h"
#include "brave/components/decentralized_dns/pref_names.h"
#include "brave/components/decentralized_dns/utils.h"
#endif

namespace {

constexpr char kTestProfileName[] = "TestProfile";
#if BUILDFLAG(DECENTRALIZED_DNS_ENABLED)
const GURL& GetDecentralizedTLDURL() {
  static const GURL url("https://brave.crypto/");
  return url;
}
#endif
const GURL& GetIPFSURI() {
  static const GURL ipfs_url(
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/wiki/"
      "Vincent_van_Gogh.html");  // NOLINT
  return ipfs_url;
}

const GURL& GetLocalhostIPGatewayURI() {
  static const GURL ipfs_url("http://127.0.0.1:8080/ipfs/QmV4FVfWR");
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
  ContentBrowserClientHelperUnitTest(
      const ContentBrowserClientHelperUnitTest&) = delete;
  ContentBrowserClientHelperUnitTest& operator=(
      const ContentBrowserClientHelperUnitTest&) = delete;
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
#if BUILDFLAG(DECENTRALIZED_DNS_ENABLED)
  bool ResolveUnstoppableURL(decentralized_dns::ResolveMethodTypes type) {
    local_state()->SetInteger(
        decentralized_dns::kUnstoppableDomainsResolveMethod,
        static_cast<int>(type));
    GURL ipfs_uri = GetDecentralizedTLDURL();
    bool result = HandleIPFSURLRewrite(&ipfs_uri, browser_context());
    EXPECT_EQ(ipfs_uri, GetDecentralizedTLDURL());
    return result;
  }
#endif
  content::WebContents* web_contents() { return web_contents_.get(); }

  // Helper that creates simple test guest profile.
  std::unique_ptr<TestingProfile> CreateGuestProfile() {
    TestingProfile::Builder profile_builder;
    profile_builder.SetGuestSession();
    return profile_builder.Build();
  }

  Profile* profile() { return profile_; }
  PrefService* local_state() { return profile_manager_->local_state()->Get(); }
  content::BrowserContext* browser_context() {
    return web_contents()->GetBrowserContext();
  }

  bool RedirectedToInternalPage(IPFSResolveMethodTypes method) {
    profile()->GetPrefs()->SetInteger(kIPFSResolveMethod,
                                      static_cast<int>(method));
    GURL ipfs_diagnostic("chrome://ipfs");
    return ipfs::HandleIPFSURLRewrite(&ipfs_diagnostic, browser_context()) &&
           ipfs_diagnostic.spec() == kIPFSWebUIURL &&
           HandleIPFSURLReverseRewrite(&ipfs_diagnostic, browser_context()) &&
           ipfs_diagnostic.spec() == kIPFSWebUIURL;
  }

 private:
  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler test_render_host_factories_;
  std::unique_ptr<content::WebContents> web_contents_;
  raw_ptr<Profile> profile_ = nullptr;
  std::unique_ptr<TestingProfileManager> profile_manager_;
  base::test::ScopedFeatureList feature_list_;
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

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPFSURLRewriteGatewayIP) {
  profile()->GetPrefs()->SetString(kIPFSPublicGatewayAddress,
                                   "http://127.0.0.1:8080/gateway");

  const GURL& localhostGateway = GetLocalhostIPGatewayURI();
  GURL ipfs_uri = localhostGateway;
  ASSERT_TRUE(HandleIPFSURLRewrite(&ipfs_uri, browser_context()));
  GURL::Replacements replacements;
  replacements.SetHostStr(kLocalhostDomain);
  EXPECT_EQ(ipfs_uri, localhostGateway.ReplaceComponents(replacements));
}

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPFSURLRewriteGatewayIPSkip) {
  profile()->GetPrefs()->SetString(kIPFSPublicGatewayAddress,
                                   "http://dweb.link/gateway");

  const GURL& localhostGateway = GetLocalhostIPGatewayURI();
  GURL ipfs_uri = localhostGateway;
  ASSERT_FALSE(HandleIPFSURLRewrite(&ipfs_uri, browser_context()));
  GURL::Replacements replacements;
  replacements.SetHostStr(kLocalhostDomain);
  EXPECT_NE(ipfs_uri, localhostGateway.ReplaceComponents(replacements));
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

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPFSURLRewriteENS) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));
  EXPECT_FALSE(decentralized_dns::IsENSResolveMethodEthereum(local_state()));
  GURL ens_uri("https://brave.eth");
  ASSERT_FALSE(HandleIPFSURLRewrite(&ens_uri, browser_context()));
  local_state()->SetInteger(
      decentralized_dns::kENSResolveMethod,
      static_cast<int>(decentralized_dns::ResolveMethodTypes::ETHEREUM));
  EXPECT_TRUE(decentralized_dns::IsENSResolveMethodEthereum(local_state()));
  ASSERT_TRUE(HandleIPFSURLRewrite(&ens_uri, browser_context()));
}

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPNSURLRewriteLocal) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));
  GURL ipns_uri(GetIPNSURI());
  ASSERT_TRUE(HandleIPFSURLRewrite(&ipns_uri, browser_context()));
}

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPFSURLReverseRewriteLocal) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));

  GURL gateway_url("http://localhost/");
  GURL::Replacements replacements;
  const std::string& port = ipfs::GetGatewayPort(chrome::GetChannel());
  replacements.SetPortStr(port);
  gateway_url = gateway_url.ReplaceComponents(replacements);

  ASSERT_EQ(ipfs::GetConfiguredBaseGateway(profile()->GetPrefs(),
                                           chrome::GetChannel()),
            gateway_url);

  std::string source = "http://test.com.ipns.localhost/#ref";
  GURL ipns_uri(GURL(source).ReplaceComponents(replacements));
  ASSERT_TRUE(HandleIPFSURLReverseRewrite(&ipns_uri, browser_context()));
  ASSERT_EQ(ipns_uri.spec(), "ipns://test.com/#ref");

  source = "http://test.com.ipns.localhost:8000/";
  ipns_uri = GURL(source);
  ASSERT_FALSE(HandleIPFSURLReverseRewrite(&ipns_uri, browser_context()));
  ASSERT_EQ(ipns_uri.spec(), source);

  ipns_uri = GURL("http://wrongcidandbaddomain.ipns.localhost/#ref");
  ipns_uri = ipns_uri.ReplaceComponents(replacements);
  source = ipns_uri.spec();
  ASSERT_FALSE(HandleIPFSURLReverseRewrite(&ipns_uri, browser_context()));
  ASSERT_EQ(ipns_uri.spec(), source);
}

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPFSURLReverseRewriteGateway) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));
  ASSERT_EQ(ipfs::GetConfiguredBaseGateway(profile()->GetPrefs(),
                                           version_info::Channel::UNKNOWN),
            GURL("https://dweb.link/"));

  std::string source = "http://test.com.ipns.localhost:8000/";
  GURL ipns_uri(source);
  ASSERT_FALSE(HandleIPFSURLReverseRewrite(&ipns_uri, browser_context()));
  ASSERT_EQ(ipns_uri.spec(), source);

  source = "https://ku2jvrakgpiqgx4j6fe.ipfs.dweb.link/";
  ipns_uri = GURL(source);
  ASSERT_FALSE(HandleIPFSURLReverseRewrite(&ipns_uri, browser_context()));
  ASSERT_EQ(ipns_uri.spec(), source);

  profile()->GetPrefs()->SetString(kIPFSPublicGatewayAddress,
                                   "http://localhost:8080");
  ASSERT_EQ(ipfs::GetConfiguredBaseGateway(profile()->GetPrefs(),
                                           version_info::Channel::UNKNOWN),
            GURL("http://localhost:8080"));

  source = "http://test.com.ipns.localhost:8000/";
  ipns_uri = GURL(source);
  ASSERT_FALSE(HandleIPFSURLReverseRewrite(&ipns_uri, browser_context()));
  ASSERT_EQ(ipns_uri.spec(), source);

  source = "https://ku2jvrakgpiqgx4j6fe.ipfs.dweb.link/";
  ipns_uri = GURL(source);
  ASSERT_FALSE(HandleIPFSURLReverseRewrite(&ipns_uri, browser_context()));
  ASSERT_EQ(ipns_uri.spec(), source);

  source = "https://ku2jvrakgpiqgx4j6fe.ipfs.dweb.link:8080/";
  ipns_uri = GURL(source);
  ASSERT_FALSE(HandleIPFSURLReverseRewrite(&ipns_uri, browser_context()));
  ASSERT_EQ(ipns_uri.spec(), source);

  source = "http://test.com.ipns.localhost:8080/#some-ref";
  ipns_uri = GURL(source);
  ASSERT_TRUE(HandleIPFSURLReverseRewrite(&ipns_uri, browser_context()));
  ASSERT_EQ(ipns_uri.spec(), "ipns://test.com/#some-ref");

  source = "https://wrongcidandbaddomain.ipns.localhost:8080/";
  ipns_uri = GURL(source);
  ASSERT_FALSE(HandleIPFSURLReverseRewrite(&ipns_uri, browser_context()));
  ASSERT_EQ(ipns_uri.spec(), source);

  ipns_uri = GURL(
      "https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      ".ipns.localhost:8080/");
  ASSERT_TRUE(HandleIPFSURLReverseRewrite(&ipns_uri, browser_context()));
  ASSERT_EQ(
      ipns_uri.spec(),
      "ipns://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/");

  ipns_uri = GURL(
      "https://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq"
      ".ipfs.localhost:8080/");
  ASSERT_TRUE(HandleIPFSURLReverseRewrite(&ipns_uri, browser_context()));
  ASSERT_EQ(
      ipns_uri.spec(),
      "ipfs://bafybeiemxf5abjwjbikoz4mc3a3dla6ual3jsgpdr4cjr3oz3evfyavhwq/");
}

TEST_F(ContentBrowserClientHelperUnitTest, HandleIPFSURLRewriteInternal) {
  ASSERT_TRUE(RedirectedToInternalPage(IPFSResolveMethodTypes::IPFS_LOCAL));
  ASSERT_TRUE(RedirectedToInternalPage(IPFSResolveMethodTypes::IPFS_GATEWAY));
  ASSERT_TRUE(RedirectedToInternalPage(IPFSResolveMethodTypes::IPFS_ASK));
  ASSERT_TRUE(RedirectedToInternalPage(IPFSResolveMethodTypes::IPFS_DISABLED));
}

#if BUILDFLAG(DECENTRALIZED_DNS_ENABLED)
TEST_F(ContentBrowserClientHelperUnitTest, HandleIPFSURLRewriteCrypto) {
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod, static_cast<int>(IPFSResolveMethodTypes::IPFS_LOCAL));
  ASSERT_TRUE(
      ResolveUnstoppableURL(decentralized_dns::ResolveMethodTypes::ETHEREUM));
  ASSERT_FALSE(ResolveUnstoppableURL(
      decentralized_dns::ResolveMethodTypes::DNS_OVER_HTTPS));
  ASSERT_FALSE(
      ResolveUnstoppableURL(decentralized_dns::ResolveMethodTypes::DISABLED));

  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_GATEWAY));
  ASSERT_FALSE(
      ResolveUnstoppableURL(decentralized_dns::ResolveMethodTypes::ETHEREUM));
  ASSERT_FALSE(ResolveUnstoppableURL(
      decentralized_dns::ResolveMethodTypes::DNS_OVER_HTTPS));
  ASSERT_FALSE(
      ResolveUnstoppableURL(decentralized_dns::ResolveMethodTypes::DISABLED));

  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(IPFSResolveMethodTypes::IPFS_DISABLED));
  ASSERT_FALSE(
      ResolveUnstoppableURL(decentralized_dns::ResolveMethodTypes::ETHEREUM));
  ASSERT_FALSE(ResolveUnstoppableURL(
      decentralized_dns::ResolveMethodTypes::DNS_OVER_HTTPS));
  ASSERT_FALSE(
      ResolveUnstoppableURL(decentralized_dns::ResolveMethodTypes::DISABLED));
}
#endif
}  // namespace ipfs
