/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "brave/browser/ipfs/ipfs_fallback_redirect_nav_data.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/media/webrtc/tab_capture_access_handler.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/channel_info.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/version_info/channel.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/test_web_contents_factory.h"
#include "content/public/test/web_contents_tester.h"
#include "content/test/test_web_contents.h"
#include "gtest/gtest.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "services/network/test/test_network_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace ipfs {

namespace {
constexpr char kCid1[] =
    "bafybeigdyrzt5sfp7udm7hu76uh7y26nf3efuylqabf3oclgtqy55fbzdi";
constexpr char kIPNSCid1[] =
    "k51qzi5uqu5dlvj2baxnqndepeb86cbk3ng7n3i46uzyxzyqj2xjonzllnv0v8";

void HeadersToRaw(std::string* headers) {
  std::replace(headers->begin(), headers->end(), '\n', '\0');
  if (!headers->empty()) {
    *headers += '\0';
  }
}

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

class FakeTestWebContents : public content::TestWebContents {
 public:
  explicit FakeTestWebContents(content::BrowserContext* browser_context)
      : TestWebContents(browser_context) {}

  static std::unique_ptr<FakeTestWebContents> Create(
      content::BrowserContext* browser_context,
      scoped_refptr<content::SiteInstance> instance) {
    std::unique_ptr<FakeTestWebContents> test_web_contents(
        new FakeTestWebContents(browser_context));
    test_web_contents->Init(CreateParams(browser_context, std::move(instance)),
                            blink::FramePolicy());
    return test_web_contents;
  }

  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override {
    WebContentsImpl::DidFinishNavigation(navigation_handle);
    if (!on_did_finish_navigation_completed.empty()) {
      auto curr_callback = on_did_finish_navigation_completed.back();
      on_did_finish_navigation_completed.pop_back();
      curr_callback.Run(navigation_handle);
    }
  }

  void SetOnDidFinishNavigationCompleted(
      base::RepeatingCallback<void(content::NavigationHandle*)> callback) {
    on_did_finish_navigation_completed.insert(
        on_did_finish_navigation_completed.begin(), callback);
  }

 private:
  std::vector<base::RepeatingCallback<void(content::NavigationHandle*)>>
      on_did_finish_navigation_completed;
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
    web_contents_ = FakeTestWebContents::Create(profile(), nullptr);
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
    profile_->GetPrefs()->SetBoolean(kIPFSAutoRedirectToConfiguredGateway,
                                     value);
  }
  void SetIpfsCompanionEnabledFlag(bool value) {
    profile_->GetPrefs()->SetBoolean(kIPFSCompanionEnabled, value);
  }
  ipfs::IPFSTabHelper* ipfs_tab_helper() {
    return ipfs::IPFSTabHelper::FromWebContents(web_contents_.get());
  }

  ipfs::FakeIPFSHostResolver* ipfs_host_resolver() {
    return ipfs_host_resolver_;
  }

  FakeTestWebContents* web_contents() { return web_contents_.get(); }

  GURL redirect_url() { return redirect_url_; }

  void ResetRedirectUrl() { redirect_url_ = GURL(); }

  void NavigateAndCommit(const GURL& url) {
    static_cast<content::TestWebContents*>(web_contents())
        ->NavigateAndCommit(url);
  }
  void NavigateAndComitFailedFailedPage(const GURL& url,
                                        const int& error_code) {
    std::unique_ptr<content::NavigationSimulator> navigation =
        content::NavigationSimulator::CreateBrowserInitiated(
            url, web_contents_.get());
    navigation->Fail(error_code);
    navigation->CommitErrorPage();
    navigation->Wait();
  }

 private:
  void OnRedirect(const GURL& gurl) { redirect_url_ = gurl; }

  GURL redirect_url_;
  content::BrowserTaskEnvironment task_environment_;
  content::RenderViewHostTestEnabler render_view_host_test_enabler_;
  TestingProfileManager profile_manager_;
  raw_ptr<TestingProfile> profile_ = nullptr;
  std::unique_ptr<FakeTestWebContents> web_contents_;
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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false, false);

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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false, false);

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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false, false);

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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false, false);

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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false, false);

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
                               false, absl::nullopt, false, false,
                               "brantly.eth", "/ipns/brantly.eth/");
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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false, false);

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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false, false);

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
  helper->DNSLinkResolved(GURL(), false, false, false);
  ASSERT_FALSE(helper->GetIPFSResolvedURL().is_valid());

  scoped_refptr<net::HttpResponseHeaders> response_headers(
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 " +
                                                     std::to_string(200)));

  response_headers->AddHeader("x-ipfs-path",
                              base::StringPrintf("/ipfs/%s", kCid1));

  helper->MaybeCheckDNSLinkRecord(response_headers.get(), false, false);
  ASSERT_FALSE(helper->ipfs_resolved_url_.is_valid());

  GURL test_url("ipns://brantly.eth/");
  helper->SetPageURLForTesting(api_server);
  helper->DNSLinkResolved(test_url, false, false, false);

  helper->MaybeCheckDNSLinkRecord(response_headers.get(), false, false);
  ASSERT_FALSE(helper->ipfs_resolved_url_.is_valid());

  helper->SetPageURLForTesting(api_server);
  helper->DNSLinkResolved(test_url, false, false, false);
  helper->UpdateDnsLinkButtonState();
  ASSERT_FALSE(helper->ipfs_resolved_url_.is_valid());

  helper->SetPageURLForTesting(api_server);
  helper->DNSLinkResolved(GURL(), false, false, false);
  helper->MaybeCheckDNSLinkRecord(response_headers.get(), false, false);
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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false, false);

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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false, false);

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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false, false);

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
  helper->MaybeCheckDNSLinkRecord(headers.get(), false, false);

  EXPECT_TRUE(ipfs_host_resolver()->resolve_called());
  ASSERT_EQ(GURL(), helper->GetIPFSResolvedURL());
}

TEST_F(IpfsTabHelperUnitTest, DetectPageLoadingError_ShowInfobar) {
  const GURL url(
      "https://ipfs.io/ipns/"
      "k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4");
  const GURL redirected_to_url(
      "ipns://k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4");

  SetIpfsCompanionEnabledFlag(false);

  web_contents()->SetOnDidFinishNavigationCompleted(
      base::BindLambdaForTesting([&](content::NavigationHandle* handler) {
        EXPECT_EQ(
            ipfs_tab_helper()->GetWebContents().GetController().GetEntryCount(),
            1);
        auto* nav_data_detected_original_url =
            IpfsFallbackRedirectNavigationData::
                FindFallbackData(web_contents());
        EXPECT_NE(nav_data_detected_original_url, nullptr);
        EXPECT_EQ(nav_data_detected_original_url->GetOriginalUrl(), url);
        EXPECT_FALSE(nav_data_detected_original_url->IsAutoRedirectBlocked());
      }));
  web_contents()->SetOnDidFinishNavigationCompleted(
      base::BindLambdaForTesting([&](content::NavigationHandle* handler) {
        EXPECT_EQ(
            ipfs_tab_helper()->GetWebContents().GetController().GetEntryCount(),
            2);
        auto* nav_data_auto_redirected = IpfsFallbackRedirectNavigationData::
            FindFallbackData(web_contents());
        EXPECT_EQ(nav_data_auto_redirected, nullptr);
      }));
  web_contents()->SetOnDidFinishNavigationCompleted(
      base::BindLambdaForTesting([&](content::NavigationHandle* handler) {
        EXPECT_EQ(
            ipfs_tab_helper()->GetWebContents().GetController().GetEntryCount(),
            2);
        auto* nav_data_after_redirect = IpfsFallbackRedirectNavigationData::
            FindFallbackData(web_contents());
        EXPECT_EQ(nav_data_after_redirect, nullptr);
      }));

  NavigateAndComitFailedFailedPage(url, 500);

  NavigateAndComitFailedFailedPage(redirected_to_url, 500);

  ipfs_tab_helper()->SetFallbackAddress(url);

  auto* nav_data_after_redirect =
      IpfsFallbackRedirectNavigationData::FindFallbackData(
          web_contents());
  EXPECT_NE(nav_data_after_redirect, nullptr);
  EXPECT_TRUE(nav_data_after_redirect->IsAutoRedirectBlocked());
  EXPECT_EQ(nav_data_after_redirect->GetOriginalUrl(), url);

  NavigateAndComitFailedFailedPage(url, 500);
}

TEST_F(IpfsTabHelperUnitTest, DetectPageLoadingError_Broken_Redirect_Chain) {
  const GURL url(
      "https://drweb.link/ipns/"
      "k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4/");
  const GURL redirected_to_url(
      "ipns://bafkreiedqfhqvarz2y4c2s3vrbrcq427sawhzbewzksegopavnmwbz4zyq");

  SetIpfsCompanionEnabledFlag(false);

  web_contents()->SetOnDidFinishNavigationCompleted(
      base::BindLambdaForTesting([&](content::NavigationHandle* handler) {
        auto* nav_data_detected_original_url =
            IpfsFallbackRedirectNavigationData::
                FindFallbackData(web_contents());
        EXPECT_NE(nav_data_detected_original_url, nullptr);
        EXPECT_EQ(nav_data_detected_original_url->GetOriginalUrl(), url);
        EXPECT_FALSE(nav_data_detected_original_url->IsAutoRedirectBlocked());
      }));
  web_contents()->SetOnDidFinishNavigationCompleted(
      base::BindLambdaForTesting([&](content::NavigationHandle* handler) {
        auto* nav_data_auto_redirected = IpfsFallbackRedirectNavigationData::
            FindFallbackData(web_contents());
        EXPECT_EQ(nav_data_auto_redirected, nullptr);
      }));

  NavigateAndComitFailedFailedPage(url, 500);

  NavigateAndComitFailedFailedPage(redirected_to_url, 500);

  auto* nav_data_after_chain_break =
      IpfsFallbackRedirectNavigationData::FindFallbackData(
          web_contents());
  EXPECT_EQ(nav_data_after_chain_break, nullptr);
}

TEST_F(IpfsTabHelperUnitTest,
       DetectPageLoadingError_Broken_Redirect_Chain_Start_New) {
  const GURL url(
      "https://drweb.link/ipns/"
      "k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4/");
  const GURL breake_redirected_to_url(
      "ipns://bafkreiedqfhqvarz2y4c2s3vrbrcq427sawhzbewzksegopavnmwbz4zyq");
  const GURL new_redirect_chain_start_url(
      "https://ipfs.io/ipfs/"
      "bafkreiedqfhqvarz2y4c2s3vrbrcq427sawhzbewzksegopavnmwbz4zyq");
  const GURL new_chain_redirected_to_url(
      "ipns://bafkreiedqfhqvarz2y4c2s3vrbrcq427sawhzbewzksegopavnmwbz4zyq");

  SetIpfsCompanionEnabledFlag(false);

  web_contents()->SetOnDidFinishNavigationCompleted(
      base::BindLambdaForTesting([&](content::NavigationHandle* handler) {
        EXPECT_EQ(
            ipfs_tab_helper()->GetWebContents().GetController().GetEntryCount(),
            1);
        auto* nav_data_detected_original_url =
            IpfsFallbackRedirectNavigationData::
                FindFallbackData(web_contents());
        EXPECT_NE(nav_data_detected_original_url, nullptr);
        EXPECT_EQ(nav_data_detected_original_url->GetOriginalUrl(), url);
        EXPECT_FALSE(nav_data_detected_original_url->IsAutoRedirectBlocked());
      }));
  web_contents()->SetOnDidFinishNavigationCompleted(
      base::BindLambdaForTesting([&](content::NavigationHandle* handler) {
        EXPECT_EQ(
            ipfs_tab_helper()->GetWebContents().GetController().GetEntryCount(),
            2);
        auto* nav_data_chain_break = IpfsFallbackRedirectNavigationData::
            FindFallbackData(web_contents());
        EXPECT_EQ(nav_data_chain_break, nullptr);
      }));
  web_contents()->SetOnDidFinishNavigationCompleted(
      base::BindLambdaForTesting([&](content::NavigationHandle* handler) {
        EXPECT_EQ(
            ipfs_tab_helper()->GetWebContents().GetController().GetEntryCount(),
            3);
        auto* nav_data_detected_new_original_url =
            IpfsFallbackRedirectNavigationData::
                FindFallbackData(web_contents());
        EXPECT_NE(nav_data_detected_new_original_url, nullptr);
        EXPECT_EQ(nav_data_detected_new_original_url->GetOriginalUrl(),
                  new_redirect_chain_start_url);
        EXPECT_FALSE(
            nav_data_detected_new_original_url->IsAutoRedirectBlocked());
      }));
  web_contents()->SetOnDidFinishNavigationCompleted(
      base::BindLambdaForTesting([&](content::NavigationHandle* handler) {
        EXPECT_EQ(
            ipfs_tab_helper()->GetWebContents().GetController().GetEntryCount(),
            4);
        auto* nav_data_new_chain_start = IpfsFallbackRedirectNavigationData::
            FindFallbackData(web_contents());
        EXPECT_EQ(nav_data_new_chain_start, nullptr);
      }));
  web_contents()->SetOnDidFinishNavigationCompleted(
      base::BindLambdaForTesting([&](content::NavigationHandle* handler) {
        EXPECT_EQ(
            ipfs_tab_helper()->GetWebContents().GetController().GetEntryCount(),
            4);
        auto* nav_data_after_redirect_new_chain =
            IpfsFallbackRedirectNavigationData::
                FindFallbackData(web_contents());
        EXPECT_EQ(nav_data_after_redirect_new_chain, nullptr);
      }));

  NavigateAndComitFailedFailedPage(url, 500);

  NavigateAndComitFailedFailedPage(breake_redirected_to_url, 500);

  auto* nav_data_after_chain_break =
      IpfsFallbackRedirectNavigationData::FindFallbackData(
          web_contents());
  EXPECT_EQ(nav_data_after_chain_break, nullptr);

  NavigateAndComitFailedFailedPage(new_redirect_chain_start_url, 500);

  NavigateAndComitFailedFailedPage(new_chain_redirected_to_url, 500);

  ipfs_tab_helper()->SetFallbackAddress(new_redirect_chain_start_url);

  auto* nav_data_after_redirect_new_chain =
      IpfsFallbackRedirectNavigationData::FindFallbackData(
          web_contents());
  EXPECT_NE(nav_data_after_redirect_new_chain, nullptr);
  EXPECT_TRUE(nav_data_after_redirect_new_chain->IsAutoRedirectBlocked());
  EXPECT_EQ(nav_data_after_redirect_new_chain->GetOriginalUrl(),
            new_redirect_chain_start_url);

  NavigateAndComitFailedFailedPage(new_redirect_chain_start_url, 500);
}

TEST_F(IpfsTabHelperUnitTest, DetectPageLoadingError_NoRedirectAsNonIPFSLink) {
  const GURL url("https://abcaddress.moc/");
  const GURL redirected_to_url("https://abcaddress.moc/");

  SetIpfsCompanionEnabledFlag(false);

  web_contents()->SetOnDidFinishNavigationCompleted(
      base::BindLambdaForTesting([&](content::NavigationHandle* handler) {
        EXPECT_EQ(
            ipfs_tab_helper()->GetWebContents().GetController().GetEntryCount(),
            1);
        auto* nav_data_detected_original_url =
            IpfsFallbackRedirectNavigationData::
                FindFallbackData(web_contents());
        EXPECT_EQ(nav_data_detected_original_url, nullptr);
      }));
  web_contents()->SetOnDidFinishNavigationCompleted(
      base::BindLambdaForTesting([&](content::NavigationHandle* handler) {
        EXPECT_EQ(
            ipfs_tab_helper()->GetWebContents().GetController().GetEntryCount(),
            1);
        auto* nav_data_detected_original_url =
            IpfsFallbackRedirectNavigationData::
                FindFallbackData(web_contents());
        EXPECT_EQ(nav_data_detected_original_url, nullptr);
      }));
  NavigateAndComitFailedFailedPage(url, 500);
  NavigateAndComitFailedFailedPage(redirected_to_url, 500);
}

TEST_F(IpfsTabHelperUnitTest, DetectPageLoadingError_IPFSCompanion_Enabled) {
  const GURL url(
      "https://drweb.link/ipns/"
      "k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4/");
  SetIpfsCompanionEnabledFlag(true);

  web_contents()->SetOnDidFinishNavigationCompleted(
      base::BindLambdaForTesting([&](content::NavigationHandle* handler) {
        EXPECT_EQ(
            ipfs_tab_helper()->GetWebContents().GetController().GetEntryCount(),
            1);
        auto* nav_data_detected_original_url =
            IpfsFallbackRedirectNavigationData::
                FindFallbackData(web_contents());
        EXPECT_EQ(nav_data_detected_original_url, nullptr);
      }));

  NavigateAndComitFailedFailedPage(url, 500);
}

}  // namespace ipfs
