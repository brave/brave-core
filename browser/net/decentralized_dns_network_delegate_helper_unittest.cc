/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/decentralized_dns_network_delegate_helper.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/decentralized_dns/constants.h"
#include "brave/components/decentralized_dns/features.h"
#include "brave/components/decentralized_dns/pref_names.h"
#include "brave/components/decentralized_dns/utils.h"
#include "chrome/test/base/scoped_testing_local_state.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/net_errors.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

using brave::ResponseCallback;

namespace decentralized_dns {

class DecentralizedDnsNetworkDelegateHelperTest : public testing::Test {
 public:
  DecentralizedDnsNetworkDelegateHelperTest()
      : local_state_(std::make_unique<ScopedTestingLocalState>(
            TestingBrowserProcess::GetGlobal())) {}

  ~DecentralizedDnsNetworkDelegateHelperTest() override = default;

  void SetUp() override {
    feature_list_.InitAndEnableFeature(features::kDecentralizedDns);
    profile_ = std::make_unique<TestingProfile>();
  }

  void TearDown() override {
    profile_.reset();
    local_state_.reset();
  }

  TestingProfile* profile() { return profile_.get(); }
  PrefService* local_state() { return local_state_->Get(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
  std::unique_ptr<ScopedTestingLocalState> local_state_;
  base::test::ScopedFeatureList feature_list_;
};

TEST_F(DecentralizedDnsNetworkDelegateHelperTest,
       DecentralizedDnsPreRedirectWork) {
  GURL url("http://brave.crypto");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);
  brave_request_info->browser_context = profile();

  // No redirect if resolve method is not set to Ethereum.
  EXPECT_FALSE(IsUnstoppableDomainsResolveMethodEthereum(local_state()));
  int rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(
      ResponseCallback(), brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  local_state()->SetInteger(kUnstoppableDomainsResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ETHEREUM));
  EXPECT_TRUE(IsUnstoppableDomainsResolveMethodEthereum(local_state()));

  // No redirect for OTR context.
  brave_request_info->browser_context =
      profile()->GetPrimaryOTRProfile(/*create_if_needed=*/true);
  rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(ResponseCallback(),
                                                          brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  brave_request_info->browser_context = profile();

  // TLD is not .crypto
  brave_request_info->request_url = GURL("http://test.com");
  rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(ResponseCallback(),
                                                          brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
  brave_request_info->request_url = url;

  rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(ResponseCallback(),
                                                          brave_request_info);
  EXPECT_EQ(rc, net::ERR_IO_PENDING);

  // No redirect if ENS resolve method is not set to Ethereum.
  EXPECT_FALSE(IsENSResolveMethodEthereum(local_state()));
  brave_request_info->request_url = GURL("http://brave.eth");
  rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(ResponseCallback(),
                                                          brave_request_info);
  EXPECT_EQ(rc, net::OK);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  local_state()->SetInteger(kENSResolveMethod,
                            static_cast<int>(ResolveMethodTypes::ETHEREUM));
  EXPECT_TRUE(IsENSResolveMethodEthereum(local_state()));
  brave_request_info->request_url = GURL("http://brave.eth");
  rc = OnBeforeURLRequest_DecentralizedDnsPreRedirectWork(ResponseCallback(),
                                                          brave_request_info);
  EXPECT_EQ(rc, net::ERR_IO_PENDING);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());
}

TEST_F(DecentralizedDnsNetworkDelegateHelperTest,
       UnstoppableDomainsRedirectWork) {
  GURL url("http://brave.crypto");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);

  // No redirect for failed requests.
  OnBeforeURLRequest_UnstoppableDomainsRedirectWork(ResponseCallback(),
                                                    brave_request_info, false,
                                                    std::vector<std::string>());
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  OnBeforeURLRequest_UnstoppableDomainsRedirectWork(
      ResponseCallback(), brave_request_info, true, std::vector<std::string>());
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  // Has both IPFS URI & fallback URL.
  std::vector<std::string> result = {
      "QmWrdNJWMbvRxxzLhojVKaBDswS4KNVM7LvjsN7QbDrvka",  // dweb.ipfs.hash
      "QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR",  // ipfs.html.value
      "",                                                // dns.A
      "",                                                // dns.AAAA
      "https://fallback1.test.com",                      // browser.redirect_url
      "https://fallback2.test.com",  // ipfs.redirect_domain.value
  };
  OnBeforeURLRequest_UnstoppableDomainsRedirectWork(
      ResponseCallback(), brave_request_info, true, result);
  EXPECT_EQ("ipfs://QmWrdNJWMbvRxxzLhojVKaBDswS4KNVM7LvjsN7QbDrvka",
            brave_request_info->new_url_spec);

  // Has legacy IPFS URI & fallback URL
  result[static_cast<int>(RecordKeys::DWEB_IPFS_HASH)] = "";
  OnBeforeURLRequest_UnstoppableDomainsRedirectWork(
      ResponseCallback(), brave_request_info, true, result);
  EXPECT_EQ("ipfs://QmbWqxBEKC3P8tqsKc98xmWNzrzDtRLMiMPL8wBuTGsMnR",
            brave_request_info->new_url_spec);

  // Has both fallback URL
  result[static_cast<int>(RecordKeys::IPFS_HTML_VALUE)] = "";
  OnBeforeURLRequest_UnstoppableDomainsRedirectWork(
      ResponseCallback(), brave_request_info, true, result);
  EXPECT_EQ("https://fallback1.test.com/", brave_request_info->new_url_spec);

  // Has legacy URL
  result[static_cast<int>(RecordKeys::BROWSER_REDIRECT_URL)] = "";
  OnBeforeURLRequest_UnstoppableDomainsRedirectWork(
      ResponseCallback(), brave_request_info, true, result);
  EXPECT_EQ("https://fallback2.test.com/", brave_request_info->new_url_spec);
}

TEST_F(DecentralizedDnsNetworkDelegateHelperTest, EnsRedirectWork) {
  GURL url("http://brantly.eth");
  auto brave_request_info = std::make_shared<brave::BraveRequestInfo>(url);

  // No redirect for failed requests.
  OnBeforeURLRequest_EnsRedirectWork(ResponseCallback(), brave_request_info,
                                     false, "");
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  OnBeforeURLRequest_EnsRedirectWork(ResponseCallback(), brave_request_info,
                                     true, "");
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  // No redirect for invalid content hash.
  std::string content_hash_encoded_string =
      "0x0000000000000000000000000000000000000000000000000000000000000020000000"
      "000000000000000000000000000000000000000000000000000000002655010170122023"
      "e0160eec32d7875c19c5ac7c03bc1f306dc260080d621454bc5f631e7310a70000000000"
      "000000000000000000000000000000000000000000";

  std::string content_hash;
  EXPECT_TRUE(brave_wallet::DecodeString(66, content_hash_encoded_string,
                                         &content_hash));
  OnBeforeURLRequest_EnsRedirectWork(ResponseCallback(), brave_request_info,
                                     true, content_hash);
  EXPECT_TRUE(brave_request_info->new_url_spec.empty());

  // Redirect for valid content hash.
  content_hash_encoded_string =
      "0x0000000000000000000000000000000000000000000000000000000000000020000000"
      "0000000000000000000000000000000000000000000000000000000026e3010170122023"
      "e0160eec32d7875c19c5ac7c03bc1f306dc260080d621454bc5f631e7310a70000000000"
      "000000000000000000000000000000000000000000";

  content_hash = "";
  EXPECT_TRUE(brave_wallet::DecodeString(66, content_hash_encoded_string,
                                         &content_hash));
  OnBeforeURLRequest_EnsRedirectWork(ResponseCallback(), brave_request_info,
                                     true, content_hash);
  EXPECT_EQ(
      brave_request_info->new_url_spec,
      "ipfs://bafybeibd4ala53bs26dvygofvr6ahpa7gbw4eyaibvrbivf4l5rr44yqu4");
}

}  // namespace decentralized_dns
