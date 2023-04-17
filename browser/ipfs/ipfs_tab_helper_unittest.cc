/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include "base/memory/raw_ptr.h"
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
#include "content/public/test/test_web_contents_factory.h"
#include "content/public/test/web_contents_tester.h"
#include "content/test/test_web_contents.h"
#include "net/http/http_response_headers.h"
#include "services/network/test/test_network_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ipfs {

class FakeIPFSHostResolver : public ipfs::IPFSHostResolver {
 public:
  explicit FakeIPFSHostResolver(network::mojom::NetworkContext& context)
      : ipfs::IPFSHostResolver(context) {}
  ~FakeIPFSHostResolver() override = default;
  void Resolve(const net::HostPortPair& host,
               const net::NetworkAnonymizationKey& anonymization_key,
               net::DnsQueryType dns_query_type,
               HostTextResultsCallback callback) override {
    resolve_called_ = true;
    if (callback)
      std::move(callback).Run(host.host(), dnslink_);
  }

  bool resolve_called() const { return resolve_called_; }

  void SetDNSLinkToRespond(const std::string& dnslink) { dnslink_ = dnslink; }

 private:
  bool resolve_called_ = false;
  std::string dnslink_;
};

class IpfsTabHelperUnitTest : public testing::Test {
 public:
  IpfsTabHelperUnitTest()
      : profile_manager_(TestingBrowserProcess::GetGlobal()) {}
  ~IpfsTabHelperUnitTest() override = default;

  void SetUp() override {
    ASSERT_TRUE(profile_manager_.SetUp());
    test_network_context_ = std::make_unique<network::TestNetworkContext>();
    profile_ = profile_manager_.CreateTestingProfile("TestProfile");
    web_contents_ = content::TestWebContents::Create(profile(), nullptr);
    auto ipfs_host_resolver =
        std::make_unique<FakeIPFSHostResolver>(*test_network_context_);
    ipfs_host_resolver_ = ipfs_host_resolver.get();
    ASSERT_TRUE(web_contents_.get());
    ASSERT_TRUE(
        ipfs::IPFSTabHelper::MaybeCreateForWebContents(web_contents_.get()));

    ipfs_tab_helper()->SetResolverForTesting(std::move(ipfs_host_resolver));
    ipfs_tab_helper()->SetRediretCallbackForTesting(base::BindRepeating(
        &IpfsTabHelperUnitTest::OnRedirect, base::Unretained(this)));
    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL);
  }
  TestingProfile* profile() { return profile_; }
  void SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes type) {
    profile_->GetPrefs()->SetInteger(kIPFSResolveMethod,
                                     static_cast<int>(type));
  }
  void SetAutoRedirectDNSLink(bool value) {
    profile_->GetPrefs()->SetBoolean(kIPFSAutoRedirectDNSLink, value);
  }
  void SetAutoRedirecIPFSResources(bool value) {
    profile_->GetPrefs()->SetBoolean(kIPFSAutoRedirectGateway, value);
  }

  ipfs::IPFSTabHelper* ipfs_tab_helper() {
    return ipfs::IPFSTabHelper::FromWebContents(web_contents_.get());
  }

  ipfs::FakeIPFSHostResolver* ipfs_host_resolver() {
    return ipfs_host_resolver_;
  }

  content::TestWebContents* web_contents() { return web_contents_.get(); }

  GURL redirect_url() { return redirect_url_; }

  void ResetRedirectUrl() { redirect_url_ = GURL(); }

 private:
  void OnRedirect(const GURL& gurl) { redirect_url_ = gurl; }

  GURL redirect_url_;
  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  TestingProfileManager profile_manager_;
  raw_ptr<TestingProfile> profile_ = nullptr;
  std::unique_ptr<content::TestWebContents> web_contents_;
  std::unique_ptr<network::TestNetworkContext> test_network_context_;
  raw_ptr<FakeIPFSHostResolver> ipfs_host_resolver_;
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

TEST_F(IpfsTabHelperUnitTest,
       TranslateUrlToIpns_When_HasDNSLinkRecord_AndXIPFSPathHeader) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(GURL("https://brantly.eth/page?query#ref"));
  helper->SetPageURLForTesting(GURL("https://brantly.eth/page?query#ref"));

  auto headers = net::HttpResponseHeaders::TryToCreate("HTTP/1.1 200 OK");
  headers->AddHeader("x-ipfs-path", "somevalue");

  ipfs_host_resolver()->SetDNSLinkToRespond("/ipns/brantly.eth/");
  helper->MaybeCheckDNSLinkRecord(headers.get());

  EXPECT_TRUE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL("ipns://brantly.eth/page?query#ref"),
            helper->GetIPFSResolvedURL());
}

TEST_F(IpfsTabHelperUnitTest,
       DoNotTranslateUrlToIpns_When_HasDNSLinkRecord_AndOriginalPageFails_400) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(GURL("https://brantly.eth/page?query#ref"));
  helper->SetPageURLForTesting(GURL("https://brantly.eth/page?query#ref"));

  auto headers = net::HttpResponseHeaders::TryToCreate("HTTP/1.1 400 Nan");
  ipfs_host_resolver()->SetDNSLinkToRespond("/ipns/brantly.eth/");
  helper->MaybeCheckDNSLinkRecord(headers.get());

  EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL(), helper->GetIPFSResolvedURL());
}

TEST_F(IpfsTabHelperUnitTest,
       TranslateUrlToIpns_When_HasDNSLinkRecord_AndOriginalPageFails_500) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(GURL("https://brantly.eth/page?query#ref"));
  helper->SetPageURLForTesting(GURL("https://brantly.eth/page?query#ref"));

  auto headers = net::HttpResponseHeaders::TryToCreate(
      "HTTP/1.1 500 Internal server error");
  ipfs_host_resolver()->SetDNSLinkToRespond("/ipns/brantly.eth/");
  helper->MaybeCheckDNSLinkRecord(headers.get());

  EXPECT_TRUE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL("ipns://brantly.eth/page?query#ref"),
            helper->GetIPFSResolvedURL());
}

TEST_F(IpfsTabHelperUnitTest,
       TranslateUrlToIpns_When_HasDNSLinkRecord_AndOriginalPageFails_505) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(GURL("https://brantly.eth/page?query#ref"));
  helper->SetPageURLForTesting(GURL("https://brantly.eth/page?query#ref"));

  auto headers = net::HttpResponseHeaders::TryToCreate(
      "HTTP/1.1 505 Version not supported");
  ipfs_host_resolver()->SetDNSLinkToRespond("/ipns/brantly.eth/");
  helper->MaybeCheckDNSLinkRecord(headers.get());

  EXPECT_TRUE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL("ipns://brantly.eth/page?query#ref"),
            helper->GetIPFSResolvedURL());
}

TEST_F(IpfsTabHelperUnitTest,
       DoNotTranslateUrlToIpns_When_NoHeader_And_NoError) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(GURL("https://brantly.eth/page?query#ref"));
  helper->SetPageURLForTesting(GURL("https://brantly.eth/page?query#ref"));

  auto headers = net::HttpResponseHeaders::TryToCreate("HTTP/1.1 200 OK");

  ipfs_host_resolver()->SetDNSLinkToRespond("");
  helper->MaybeCheckDNSLinkRecord(headers.get());

  EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL(), helper->GetIPFSResolvedURL());
}

TEST_F(IpfsTabHelperUnitTest, DNSLinkRecordResolved_AutoRedirectDNSLink) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);
  GURL gateway = ipfs::GetConfiguredBaseGateway(profile()->GetPrefs(),
                                                chrome::GetChannel());
  web_contents()->NavigateAndCommit(GURL("https://brantly.eth/page?query#ref"));

  helper->SetPageURLForTesting(GURL("https://brantly.eth/page?query#ref"));
  helper->HostResolvedCallback(GURL("https://brantly.eth/page?query#ref"),
                               GURL("https://brantly.eth/page?query#ref"),
                               false, absl::nullopt, "brantly.eth",
                               "/ipns/brantly.eth/");
  ASSERT_EQ(GURL("ipns://brantly.eth/page?query#ref"),
            helper->GetIPFSResolvedURL());
}

TEST_F(IpfsTabHelperUnitTest, XIpfsPathHeaderUsed_IfNoDnsLinkRecord_IPFS) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(GURL("https://brantly.eth/page?query#ref"));
  helper->SetPageURLForTesting(GURL("https://brantly.eth/page?query#ref"));
  SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
  GURL gateway = ipfs::GetConfiguredBaseGateway(profile()->GetPrefs(),
                                                chrome::GetChannel());

  auto headers = net::HttpResponseHeaders::TryToCreate("HTTP/1.1 200 OK");
  headers->AddHeader("x-ipfs-path", "/ipfs/bafy");

  ipfs_host_resolver()->SetDNSLinkToRespond("");
  helper->MaybeCheckDNSLinkRecord(headers.get());

  EXPECT_TRUE(ipfs_host_resolver()->resolve_called());
  GURL resolved_url = helper->GetIPFSResolvedURL();

  EXPECT_EQ(resolved_url.spec(), "ipfs://bafy?query#ref");
}

TEST_F(IpfsTabHelperUnitTest, XIpfsPathHeaderUsed_IfNoDnsLinkRecord_IPNS) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(GURL("https://brantly.eth/page?query#ref"));
  helper->SetPageURLForTesting(GURL("https://brantly.eth/page?query#ref"));
  SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
  GURL gateway = ipfs::GetConfiguredBaseGateway(profile()->GetPrefs(),
                                                chrome::GetChannel());

  auto headers = net::HttpResponseHeaders::TryToCreate("HTTP/1.1 200 OK");
  headers->AddHeader("x-ipfs-path", "/ipns/brantly.eth/");

  ipfs_host_resolver()->SetDNSLinkToRespond("");
  helper->MaybeCheckDNSLinkRecord(headers.get());

  EXPECT_TRUE(ipfs_host_resolver()->resolve_called());
  GURL resolved_url = helper->GetIPFSResolvedURL();

  EXPECT_EQ(resolved_url, GURL("ipns://brantly.eth/?query#ref"));
}

TEST_F(IpfsTabHelperUnitTest, ResolveXIPFSPathUrl) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  {
    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
    GURL gateway = ipfs::GetConfiguredBaseGateway(profile()->GetPrefs(),
                                                  chrome::GetChannel());
    GURL url = helper->ResolveXIPFSPathUrl("/ipfs/bafy");
    EXPECT_EQ(url, GURL("ipfs://bafy"));
  }

  {
    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL);
    GURL gateway = ipfs::GetConfiguredBaseGateway(profile()->GetPrefs(),
                                                  chrome::GetChannel());
    GURL url = helper->ResolveXIPFSPathUrl("/ipfs/bafy");
    EXPECT_EQ(url, GURL("ipfs://bafy"));
  }

  {
    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_ASK);
    GURL gateway = ipfs::GetConfiguredBaseGateway(profile()->GetPrefs(),
                                                  chrome::GetChannel());
    GURL url = helper->ResolveXIPFSPathUrl("/ipfs/bafy");
    EXPECT_EQ(url, GURL("ipfs://bafy"));
  }
}

TEST_F(IpfsTabHelperUnitTest, GatewayResolving) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  GURL api_server = GetAPIServer(chrome::GetChannel());
  helper->SetPageURLForTesting(api_server);
  helper->DNSLinkResolved(GURL(), false);
  ASSERT_FALSE(helper->GetIPFSResolvedURL().is_valid());

  scoped_refptr<net::HttpResponseHeaders> response_headers(
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 " +
                                                     std::to_string(200)));

  response_headers->AddHeader("x-ipfs-path", "/ipfs/bafy");

  helper->MaybeCheckDNSLinkRecord(response_headers.get());
  ASSERT_FALSE(helper->ipfs_resolved_url_.is_valid());

  GURL test_url("ipns://brantly.eth/");
  helper->SetPageURLForTesting(api_server);
  helper->DNSLinkResolved(test_url, false);

  helper->MaybeCheckDNSLinkRecord(response_headers.get());
  ASSERT_FALSE(helper->ipfs_resolved_url_.is_valid());

  helper->SetPageURLForTesting(api_server);
  helper->DNSLinkResolved(test_url, false);
  helper->UpdateDnsLinkButtonState();
  ASSERT_FALSE(helper->ipfs_resolved_url_.is_valid());

  helper->SetPageURLForTesting(api_server);
  helper->DNSLinkResolved(GURL(), false);
  helper->MaybeCheckDNSLinkRecord(response_headers.get());
  ASSERT_FALSE(helper->ipfs_resolved_url_.is_valid());
}

TEST_F(IpfsTabHelperUnitTest, GatewayLikeUrlParsed_AutoRedirectEnabled) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  SetAutoRedirecIPFSResources(true);

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
    helper->SetPageURLForTesting(GURL("https://ipfs.io/ipfs/bafy1/?query#ref"));

    web_contents()->NavigateAndCommit(
        GURL("https://ipfs.io/ipfs/bafy1/?query#ref"));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL("ipfs://bafy1?query#ref"));
  }

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
    helper->SetPageURLForTesting(GURL("https://bafy1.ipfs.ipfs.io?query#ref"));

    web_contents()->NavigateAndCommit(
        GURL("https://bafy1.ipfs.ipfs.io?query#ref"));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL("ipfs://bafy1/?query#ref"));
  }

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_ASK);
    helper->SetPageURLForTesting(GURL("https://ipfs.io/ipfs/bafy2/?query#ref"));

    web_contents()->NavigateAndCommit(
        GURL("https://ipfs.io/ipfs/bafy2/?query#ref"));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL("ipfs://bafy2?query#ref"));
  }

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL);
    helper->SetPageURLForTesting(GURL("https://ipfs.io/ipfs/bafy3/?query#ref"));

    web_contents()->NavigateAndCommit(
        GURL("https://ipfs.io/ipfs/bafy3/?query#ref"));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL("ipfs://bafy3?query#ref"));
  }

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_DISABLED);
    helper->SetPageURLForTesting(GURL("https://ipfs.io/ipfs/bafy3/?query#ref"));

    web_contents()->NavigateAndCommit(
        GURL("https://ipfs.io/ipfs/bafy3/?query#ref"));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL());
  }
}

TEST_F(IpfsTabHelperUnitTest,
       GatewayLikeUrlParsed_AutoRedirectEnabled_WrongFormat) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  SetAutoRedirecIPFSResources(true);

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
    helper->SetPageURLForTesting(
        GURL("https://ipfs.io/ipxxs/bafy1/?query#ref"));

    web_contents()->NavigateAndCommit(
        GURL("https://ipfs.io/ipxxs/bafy1/?query#ref"));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL());
  }

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
    helper->SetPageURLForTesting(GURL("https://bafy1.ipxxs.ipfs.io?query#ref"));

    web_contents()->NavigateAndCommit(
        GURL("https://bafy1.ipxxs.ipfs.io?query#ref"));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL());
  }
}

TEST_F(IpfsTabHelperUnitTest,
       GatewayLikeUrlParsed_AutoRedirectEnabled_ConfiguredGatewayIgnored) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  SetAutoRedirecIPFSResources(true);

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
    SetIPFSDefaultGatewayForTest(GURL("https://a.com/"));

    helper->SetPageURLForTesting(GURL("https://a.com/ipfs/bafy"));

    web_contents()->NavigateAndCommit(GURL("https://a.com/ipfs/bafy"));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL());
  }
}

TEST_F(IpfsTabHelperUnitTest, GatewayLikeUrlParsed_AutoRedirectDisabled) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  SetAutoRedirecIPFSResources(false);

  {
    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);

    helper->SetPageURLForTesting(GURL("https://ipfs.io/ipfs/bafy1/?query#ref"));
    web_contents()->NavigateAndCommit(
        GURL("https://ipfs.io/ipfs/bafy1/?query#ref"));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL());
    EXPECT_EQ(ipfs_tab_helper()->ipfs_resolved_url_,
              GURL("ipfs://bafy1?query#ref"));
  }
}

TEST_F(IpfsTabHelperUnitTest, GatewayIPNS_ResolveUrl) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(
      GURL("https://ipfs.io/ipns/brantly.eth/page?query#ref"));
  helper->SetPageURLForTesting(
      GURL("https://ipfs.io/ipns/brantly.eth/page?query#ref"));

  ipfs_host_resolver()->SetDNSLinkToRespond("/ipns/brantly.eth/");
  auto headers = net::HttpResponseHeaders::TryToCreate("HTTP/1.1 200 OK");
  helper->MaybeCheckDNSLinkRecord(headers.get());

  EXPECT_TRUE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL("ipns://brantly.eth/page?query#ref"),
            helper->GetIPFSResolvedURL());
}

TEST_F(IpfsTabHelperUnitTest, GatewayIPNS_Redirect) {
  SetAutoRedirecIPFSResources(true);
  SetAutoRedirectDNSLink(false);

  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(
      GURL("https://ipfs.io/ipns/brantly-eth/page?query#ref"));
  helper->SetPageURLForTesting(
      GURL("https://ipfs.io/ipns/brantly-eth/page?query#ref"));

  ipfs_host_resolver()->SetDNSLinkToRespond("x");
  auto headers = net::HttpResponseHeaders::TryToCreate("HTTP/1.1 200 OK");
  helper->MaybeCheckDNSLinkRecord(headers.get());

  EXPECT_TRUE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL("ipns://brantly.eth/page?query#ref"), redirect_url());
}

TEST_F(IpfsTabHelperUnitTest, GatewayIPNS_No_Redirect_WhenNoDnsLink) {
  SetAutoRedirecIPFSResources(true);
  SetAutoRedirectDNSLink(false);

  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(
      GURL("https://ipfs.io/ipns/brantly-eth/page?query#ref"));
  helper->SetPageURLForTesting(
      GURL("https://ipfs.io/ipns/brantly-eth/page?query#ref"));

  ipfs_host_resolver()->SetDNSLinkToRespond("");
  auto headers = net::HttpResponseHeaders::TryToCreate("HTTP/1.1 200 OK");
  helper->MaybeCheckDNSLinkRecord(headers.get());

  EXPECT_TRUE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL(), redirect_url());
}

TEST_F(IpfsTabHelperUnitTest, GatewayIPNS_Redirect_LibP2PKey) {
  SetAutoRedirecIPFSResources(true);
  SetAutoRedirectDNSLink(false);

  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(
      GURL("https://ipfs.io/ipns/"
           "k51qzi5uqu5dlvj2baxnqndepeb86cbk3ng7n3i46uzyxzyqj2xjonzllnv0v8/"
           "page?query#ref"));
  helper->SetPageURLForTesting(
      GURL("https://ipfs.io/ipns/"
           "k51qzi5uqu5dlvj2baxnqndepeb86cbk3ng7n3i46uzyxzyqj2xjonzllnv0v8/"
           "page?query#ref"));

  EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL("ipns://"
                 "k51qzi5uqu5dlvj2baxnqndepeb86cbk3ng7n3i46uzyxzyqj2xjonzllnv0v"
                 "8/page?query#ref"),
            redirect_url());
}

TEST_F(IpfsTabHelperUnitTest, GatewayIPNS_Redirect_LibP2PKey_NoAutoRedirect) {
  SetAutoRedirecIPFSResources(false);
  SetAutoRedirectDNSLink(false);

  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(
      GURL("https://ipfs.io/ipns/"
           "k51qzi5uqu5dlvj2baxnqndepeb86cbk3ng7n3i46uzyxzyqj2xjonzllnv0v8/"
           "page?query#ref"));
  helper->SetPageURLForTesting(
      GURL("https://ipfs.io/ipns/"
           "k51qzi5uqu5dlvj2baxnqndepeb86cbk3ng7n3i46uzyxzyqj2xjonzllnv0v8/"
           "page?query#ref"));

  EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL("ipns://"
                 "k51qzi5uqu5dlvj2baxnqndepeb86cbk3ng7n3i46uzyxzyqj2xjonzllnv0v"
                 "8/page?query#ref"),
            helper->GetIPFSResolvedURL());
}

TEST_F(IpfsTabHelperUnitTest, GatewayIPNS_NoRedirect_WhenNoDnsLinkRecord) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(
      GURL("https://ipfs.io/ipns/brantly.eth/page?query#ref"));
  helper->SetPageURLForTesting(
      GURL("https://ipfs.io/ipns/brantly.eth/page?query#ref"));

  auto headers = net::HttpResponseHeaders::TryToCreate("HTTP/1.1 200 OK");
  helper->MaybeCheckDNSLinkRecord(headers.get());

  EXPECT_TRUE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL(), helper->GetIPFSResolvedURL());
}

}  // namespace ipfs
