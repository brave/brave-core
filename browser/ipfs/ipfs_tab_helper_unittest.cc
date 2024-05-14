/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include <optional>

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

namespace {
constexpr char kCid1[] =
    "bafybeigdyrzt5sfp7udm7hu76uh7y26nf3efuylqabf3oclgtqy55fbzdi";
constexpr char kIPNSCid1[] =
    "k51qzi5uqu5dlvj2baxnqndepeb86cbk3ng7n3i46uzyxzyqj2xjonzllnv0v8";
}  // namespace

class FakeIPFSHostResolver : public ipfs::IPFSHostResolver {
 public:
  FakeIPFSHostResolver() : ipfs::IPFSHostResolver(nullptr) {}
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
    auto ipfs_host_resolver = std::make_unique<FakeIPFSHostResolver>();
    ipfs_host_resolver_ = ipfs_host_resolver.get();
    ipfs_host_resolver_->SetNetworkContextForTesting(
        test_network_context_.get());
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

  void SetAutoRedirectToConfiguredGateway(bool value) {
    // profile_->GetPrefs()->SetBoolean(kIPFSAutoRedirectToConfiguredGateway,
    //                                  value);
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

  ASSERT_TRUE(
      helper->CanResolveURL(GURL("https://"
                                 "bafybeigdyrzt5sfp7udm7hu76uh7y26nf3efuylqabf3"
                                 "oclgtqy55fbzdi.ipfs.dweb.link/")));
  profile()->GetPrefs()->SetInteger(
      kIPFSResolveMethod,
      static_cast<int>(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY));
  ASSERT_FALSE(
      helper->CanResolveURL(GURL("https://"
                                 "bafybeigdyrzt5sfp7udm7hu76uh7y26nf3efuylqabf3"
                                 "oclgtqy55fbzdi.ipfs.dweb.link/")));
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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false);

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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false);

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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false);

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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false);

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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false);

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
                               false, std::nullopt, false, "brantly.eth",
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
  headers->AddHeader("x-ipfs-path", base::StringPrintf("/ipfs/%s", kCid1));

  ipfs_host_resolver()->SetDNSLinkToRespond("");
  helper->MaybeCheckDNSLinkRecord(headers.get(), false);

  EXPECT_TRUE(ipfs_host_resolver()->resolve_called());
  GURL resolved_url = helper->GetIPFSResolvedURL();

  EXPECT_EQ(resolved_url.spec(),
            base::StringPrintf("ipfs://%s?query#ref", kCid1));
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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false);

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
    GURL url =
        helper->ResolveXIPFSPathUrl(base::StringPrintf("/ipfs/%s", kCid1));
    EXPECT_EQ(url, GURL(base::StringPrintf("ipfs://%s", kCid1)));
  }

  {
    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL);
    GURL gateway = ipfs::GetConfiguredBaseGateway(profile()->GetPrefs(),
                                                  chrome::GetChannel());
    GURL url =
        helper->ResolveXIPFSPathUrl(base::StringPrintf("/ipfs/%s", kCid1));
    EXPECT_EQ(url, GURL(base::StringPrintf("ipfs://%s", kCid1)));
  }

  {
    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_ASK);
    GURL gateway = ipfs::GetConfiguredBaseGateway(profile()->GetPrefs(),
                                                  chrome::GetChannel());
    GURL url =
        helper->ResolveXIPFSPathUrl(base::StringPrintf("/ipfs/%s", kCid1));
    EXPECT_EQ(url, GURL(base::StringPrintf("ipfs://%s", kCid1)));
  }
}

TEST_F(IpfsTabHelperUnitTest, GatewayResolving) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  GURL api_server = GetAPIServer(chrome::GetChannel());
  helper->SetPageURLForTesting(api_server);
  helper->DNSLinkResolved(GURL(), false, false);
  ASSERT_FALSE(helper->GetIPFSResolvedURL().is_valid());

  scoped_refptr<net::HttpResponseHeaders> response_headers(
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 " +
                                                     std::to_string(200)));

  response_headers->AddHeader("x-ipfs-path",
                              base::StringPrintf("/ipfs/%s", kCid1));

  helper->MaybeCheckDNSLinkRecord(response_headers.get(), false);
  ASSERT_FALSE(helper->ipfs_resolved_url_.is_valid());

  GURL test_url("ipns://brantly.eth/");
  helper->SetPageURLForTesting(api_server);
  helper->DNSLinkResolved(test_url, false, false);

  helper->MaybeCheckDNSLinkRecord(response_headers.get(), false);
  ASSERT_FALSE(helper->ipfs_resolved_url_.is_valid());

  helper->SetPageURLForTesting(api_server);
  helper->DNSLinkResolved(test_url, false, false);
  helper->UpdateDnsLinkButtonState();
  ASSERT_FALSE(helper->ipfs_resolved_url_.is_valid());

  helper->SetPageURLForTesting(api_server);
  helper->DNSLinkResolved(GURL(), false, false);
  helper->MaybeCheckDNSLinkRecord(response_headers.get(), false);
  ASSERT_FALSE(helper->ipfs_resolved_url_.is_valid());
}

TEST_F(IpfsTabHelperUnitTest, GatewayLikeUrlParsed_AutoRedirectEnabled) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  SetAutoRedirectToConfiguredGateway(true);

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
    helper->SetPageURLForTesting(
        GURL("https://ipfs.io/ipfs/"
             "bafybeigdyrzt5sfp7udm7hu76uh7y26nf3efuylqabf3oclgtqy55fbzdi/"
             "?query#ref"));

    web_contents()->NavigateAndCommit(
        GURL(base::StringPrintf("https://ipfs.io/ipfs/"
                                "%s/"
                                "?query#ref",
                                kCid1)));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL("ipfs://"
                                   "bafybeigdyrzt5sfp7udm7hu76uh7y26nf3efuylqab"
                                   "f3oclgtqy55fbzdi?query#ref"));
  }

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
    helper->SetPageURLForTesting(
        GURL(base::StringPrintf("https://"
                                "%s.ipfs."
                                "ipfs.io?query#ref",
                                kCid1)));

    web_contents()->NavigateAndCommit(
        GURL(base::StringPrintf("https://"
                                "%s.ipfs."
                                "ipfs.io?query#ref",
                                kCid1)));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL("ipfs://"
                                   "bafybeigdyrzt5sfp7udm7hu76uh7y26nf3efuylqab"
                                   "f3oclgtqy55fbzdi/?query#ref"));
  }

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_ASK);
    helper->SetPageURLForTesting(
        GURL(base::StringPrintf("https://ipfs.io/ipfs/"
                                "%s/"
                                "?query#ref",
                                kCid1)));

    web_contents()->NavigateAndCommit(
        GURL(base::StringPrintf("https://ipfs.io/ipfs/"
                                "%s/"
                                "?query#ref",
                                kCid1)));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL("ipfs://"
                                   "bafybeigdyrzt5sfp7udm7hu76uh7y26nf3efuylqab"
                                   "f3oclgtqy55fbzdi?query#ref"));
  }

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_LOCAL);
    helper->SetPageURLForTesting(
        GURL(base::StringPrintf("https://ipfs.io/ipfs/"
                                "%s/"
                                "?query#ref",
                                kCid1)));

    web_contents()->NavigateAndCommit(
        GURL(base::StringPrintf("https://ipfs.io/ipfs/"
                                "%s/"
                                "?query#ref",
                                kCid1)));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL("ipfs://"
                                   "bafybeigdyrzt5sfp7udm7hu76uh7y26nf3efuylqab"
                                   "f3oclgtqy55fbzdi?query#ref"));
  }

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_DISABLED);
    helper->SetPageURLForTesting(
        GURL(base::StringPrintf("https://ipfs.io/ipfs/"
                                "%s/"
                                "?query#ref",
                                kCid1)));

    web_contents()->NavigateAndCommit(
        GURL(base::StringPrintf("https://ipfs.io/ipfs/"
                                "%s/"
                                "?query#ref",
                                kCid1)));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL());
  }
}

TEST_F(IpfsTabHelperUnitTest,
       GatewayLikeUrlParsed_AutoRedirectEnabled_WrongFormat) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  SetAutoRedirectToConfiguredGateway(true);

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
    helper->SetPageURLForTesting(
        GURL(base::StringPrintf("https://ipfs.io/ipxxs/"
                                "%s/"
                                "?query#ref",
                                kCid1)));

    web_contents()->NavigateAndCommit(
        GURL(base::StringPrintf("https://ipfs.io/ipxxs/"
                                "%s/"
                                "?query#ref",
                                kCid1)));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL());
  }

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
    helper->SetPageURLForTesting(
        GURL(base::StringPrintf("https://"
                                "%s."
                                "ipxxs.ipfs.io?query#ref",
                                kCid1)));

    web_contents()->NavigateAndCommit(
        GURL(base::StringPrintf("https://"
                                "%s."
                                "ipxxs.ipfs.io?query#ref",
                                kCid1)));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL());
  }

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
    helper->SetPageURLForTesting(
        GURL("https://"
             "bafy."
             "ipfs.ipfs.io?query#ref"));

    web_contents()->NavigateAndCommit(
        GURL("https://"
             "bafy."
             "ipfs.ipfs.io?query#ref"));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL());
  }

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
    helper->SetPageURLForTesting(
        GURL("https://ipfs.io/ipfs/"
             "bafy/"
             "?query#ref"));

    web_contents()->NavigateAndCommit(
        GURL("https://ipfs.io/ipfs/"
             "bafy/"
             "?query#ref"));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL());
  }
}

TEST_F(IpfsTabHelperUnitTest,
       GatewayLikeUrlParsed_AutoRedirectEnabled_ConfiguredGatewayIgnored) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  SetAutoRedirectToConfiguredGateway(true);

  {
    ResetRedirectUrl();

    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);
    SetIPFSDefaultGatewayForTest(GURL("https://a.com/"));

    helper->SetPageURLForTesting(
        GURL(base::StringPrintf("https://a.com/ipfs/"
                                "%s",
                                kCid1)));

    web_contents()->NavigateAndCommit(
        GURL(base::StringPrintf("https://a.com/ipfs/"
                                "%s",
                                kCid1)));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL());
  }
}

TEST_F(IpfsTabHelperUnitTest, GatewayLikeUrlParsed_AutoRedirectDisabled) {
  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  SetAutoRedirectToConfiguredGateway(false);

  {
    SetIPFSResolveMethodPref(ipfs::IPFSResolveMethodTypes::IPFS_GATEWAY);

    helper->SetPageURLForTesting(
        GURL(base::StringPrintf("https://ipfs.io/ipfs/"
                                "%s/"
                                "?query#ref",
                                kCid1)));
    web_contents()->NavigateAndCommit(
        GURL(base::StringPrintf("https://ipfs.io/ipfs/"
                                "%s/"
                                "?query#ref",
                                kCid1)));

    EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
    EXPECT_EQ(redirect_url(), GURL());
    EXPECT_EQ(ipfs_tab_helper()->ipfs_resolved_url_,
              GURL(base::StringPrintf("ipfs://"
                                      "%s"
                                      "?query#ref",
                                      kCid1)));
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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false);

  EXPECT_TRUE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL("ipns://brantly.eth/page?query#ref"),
            helper->GetIPFSResolvedURL());
}

TEST_F(IpfsTabHelperUnitTest, GatewayIPNS_Redirect) {
  SetAutoRedirectToConfiguredGateway(true);

  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(
      GURL("https://ipfs.io/ipns/brantly-eth/page?query#ref"));
  helper->SetPageURLForTesting(
      GURL("https://ipfs.io/ipns/brantly-eth/page?query#ref"));

  ipfs_host_resolver()->SetDNSLinkToRespond("x");
  auto headers = net::HttpResponseHeaders::TryToCreate("HTTP/1.1 200 OK");
  helper->MaybeCheckDNSLinkRecord(headers.get(), false);

  EXPECT_TRUE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL("ipns://brantly.eth/page?query#ref"), redirect_url());
}

TEST_F(IpfsTabHelperUnitTest, GatewayIPNS_No_Redirect_WhenNoDnsLink) {
  SetAutoRedirectToConfiguredGateway(true);

  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(
      GURL("https://ipfs.io/ipns/brantly-eth/page?query#ref"));
  helper->SetPageURLForTesting(
      GURL("https://ipfs.io/ipns/brantly-eth/page?query#ref"));

  ipfs_host_resolver()->SetDNSLinkToRespond("");
  auto headers = net::HttpResponseHeaders::TryToCreate("HTTP/1.1 200 OK");
  helper->MaybeCheckDNSLinkRecord(headers.get(), false);

  EXPECT_TRUE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL(), redirect_url());
}

TEST_F(IpfsTabHelperUnitTest, GatewayIPNS_Redirect_LibP2PKey) {
  SetAutoRedirectToConfiguredGateway(true);

  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(GURL(
      base::StringPrintf("https://ipfs.io/ipns/%s/page?query#ref", kIPNSCid1)));
  helper->SetPageURLForTesting(GURL(
      base::StringPrintf("https://ipfs.io/ipns/%s/page?query#ref", kIPNSCid1)));

  EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL(base::StringPrintf("ipns://%s/page?query#ref", kIPNSCid1)),
            redirect_url());
}

TEST_F(IpfsTabHelperUnitTest, GatewayIPNS_Redirect_LibP2PKey_NoAutoRedirect) {
  SetAutoRedirectToConfiguredGateway(false);

  auto* helper = ipfs_tab_helper();
  ASSERT_TRUE(helper);

  web_contents()->NavigateAndCommit(GURL(
      base::StringPrintf("https://ipfs.io/ipns/%s/page?query#ref", kIPNSCid1)));
  helper->SetPageURLForTesting(GURL(
      base::StringPrintf("https://ipfs.io/ipns/%s/page?query#ref", kIPNSCid1)));

  EXPECT_FALSE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL(base::StringPrintf("ipns://%s/page?query#ref", kIPNSCid1)),
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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false);

  EXPECT_TRUE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL(), helper->GetIPFSResolvedURL());
}

}  // namespace ipfs
