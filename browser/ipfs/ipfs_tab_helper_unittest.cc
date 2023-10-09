/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ipfs/ipfs_tab_helper.h"

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/test/bind.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/channel_info.h"
#include "chrome/test/base/testing_browser_process.h"
#include "chrome/test/base/testing_profile.h"
#include "chrome/test/base/testing_profile_manager.h"
#include "components/version_info/channel.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/test_browser_context.h"
#include "content/public/test/test_renderer_host.h"
#include "content/public/test/test_web_contents_factory.h"
#include "content/public/test/web_contents_tester.h"
#include "content/test/test_web_contents.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "services/network/test/test_network_context.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace content {
class NavigationHandleMock : public content::NavigationHandle {
 public:
  MOCK_METHOD(int64_t, GetNavigationId, (), (override));
  MOCK_METHOD(ukm::SourceId, GetNextPageUkmSourceId, (), (override));
  MOCK_METHOD(const GURL&, GetURL, (), (override));
  MOCK_METHOD(SiteInstance*, GetStartingSiteInstance, (), (override));
  MOCK_METHOD(SiteInstance*, GetSourceSiteInstance, (), (override));
  MOCK_METHOD(bool, IsInMainFrame, (), (const override));
  MOCK_METHOD(bool, IsInPrimaryMainFrame, (), (const override));
  MOCK_METHOD(bool, IsInOutermostMainFrame, (), (override));
  MOCK_METHOD(bool, IsInPrerenderedMainFrame, (), (const override));
  MOCK_METHOD(bool, IsPrerenderedPageActivation, (), (const override));
  MOCK_METHOD(bool, IsInFencedFrameTree, (), (const override));
  MOCK_METHOD(FrameType, GetNavigatingFrameType, (), (const override));
  MOCK_METHOD(bool, IsRendererInitiated, (), (override));
  MOCK_METHOD(blink::mojom::NavigationInitiatorActivationAndAdStatus,
              GetNavigationInitiatorActivationAndAdStatus,
              (),
              (override));
  MOCK_METHOD(bool, IsSameOrigin, (), (override));
  MOCK_METHOD(int, GetFrameTreeNodeId, (), (override));
  MOCK_METHOD(RenderFrameHost*, GetParentFrame, (), (override));
  MOCK_METHOD(RenderFrameHost*, GetParentFrameOrOuterDocument, (), (override));
  MOCK_METHOD(base::TimeTicks, NavigationStart, (), (override));
  MOCK_METHOD(base::TimeTicks, NavigationInputStart, (), (override));
  MOCK_METHOD(const NavigationHandleTiming&,
              GetNavigationHandleTiming,
              (),
              (override));
  MOCK_METHOD(bool, WasStartedFromContextMenu, (), (override));
  MOCK_METHOD(const GURL&, GetSearchableFormURL, (), (override));
  MOCK_METHOD(const std::string&, GetSearchableFormEncoding, (), (override));
  MOCK_METHOD(ReloadType, GetReloadType, (), (override));
  MOCK_METHOD(RestoreType, GetRestoreType, (), (override));
  MOCK_METHOD(const GURL&, GetBaseURLForDataURL, (), (override));
  MOCK_METHOD(bool, IsPost, (), (override));
  MOCK_METHOD(const blink::mojom::Referrer&, GetReferrer, (), (override));
  MOCK_METHOD(void,
              SetReferrer,
              (blink::mojom::ReferrerPtr referrer),
              (override));
  MOCK_METHOD(bool, HasUserGesture, (), (override));
  MOCK_METHOD(ui::PageTransition, GetPageTransition, (), (override));
  MOCK_METHOD(NavigationUIData*, GetNavigationUIData, (), (override));
  MOCK_METHOD(bool, IsExternalProtocol, (), (override));
  MOCK_METHOD(bool, IsServedFromBackForwardCache, (), (override));
  MOCK_METHOD(bool, IsPageActivation, (), (const override));
  MOCK_METHOD(net::Error, GetNetErrorCode, (), (override));
  MOCK_METHOD(RenderFrameHost*, GetRenderFrameHost, (), (const override));
  MOCK_METHOD(GlobalRenderFrameHostId,
              GetPreviousRenderFrameHostId,
              (),
              (override));
  MOCK_METHOD(int, GetExpectedRenderProcessHostId, (), (override));
  MOCK_METHOD(bool, IsSameDocument, (), (const override));
  MOCK_METHOD(bool, WasServerRedirect, (), (override));
  MOCK_METHOD(const std::vector<GURL>&, GetRedirectChain, (), (override));
  MOCK_METHOD(bool, HasCommitted, (), (const override));
  MOCK_METHOD(bool, IsErrorPage, (), (const override));
  MOCK_METHOD(bool, HasSubframeNavigationEntryCommitted, (), (override));
  MOCK_METHOD(bool, DidReplaceEntry, (), (override));
  MOCK_METHOD(bool, ShouldUpdateHistory, (), (override));
  MOCK_METHOD(const GURL&, GetPreviousPrimaryMainFrameURL, (), (override));
  MOCK_METHOD(net::IPEndPoint, GetSocketAddress, (), (override));
  MOCK_METHOD(const net::HttpRequestHeaders&,
              GetRequestHeaders,
              (),
              (override));
  MOCK_METHOD(void,
              RemoveRequestHeader,
              (const std::string& header_name),
              (override));
  MOCK_METHOD(void,
              SetRequestHeader,
              (const std::string& header_name, const std::string& header_value),
              (override));
  MOCK_METHOD(void,
              SetCorsExemptRequestHeader,
              (const std::string& header_name, const std::string& header_value),
              (override));
  MOCK_METHOD(
      void,
      SetLCPPNavigationHint,
      (const blink::mojom::LCPCriticalPathPredictorNavigationTimeHint& hint),
      (override));
  MOCK_METHOD(
      const blink::mojom::LCPCriticalPathPredictorNavigationTimeHintPtr&,
      GetLCPPNavigationHint,
      (),
      (override));
  MOCK_METHOD(const net::HttpResponseHeaders*,
              GetResponseHeaders,
              (),
              (override));
  MOCK_METHOD(net::HttpResponseInfo::ConnectionInfo,
              GetConnectionInfo,
              (),
              (override));
  MOCK_METHOD(const absl::optional<net::SSLInfo>&, GetSSLInfo, (), (override));
  MOCK_METHOD(const absl::optional<net::AuthChallengeInfo>&,
              GetAuthChallengeInfo,
              (),
              (override));
  MOCK_METHOD(net::ResolveErrorInfo, GetResolveErrorInfo, (), (override));
  MOCK_METHOD(net::IsolationInfo, GetIsolationInfo, (), (override));
  MOCK_METHOD(const GlobalRequestID&, GetGlobalRequestID, (), (override));
  MOCK_METHOD(bool, IsDownload, (), (override));
  MOCK_METHOD(bool, IsFormSubmission, (), (override));
  MOCK_METHOD(bool, WasInitiatedByLinkClick, (), (override));
  MOCK_METHOD(bool, IsSignedExchangeInnerResponse, (), (override));
  MOCK_METHOD(bool,
              HasPrefetchedAlternativeSubresourceSignedExchange,
              (),
              (override));
  MOCK_METHOD(bool, WasResponseCached, (), (override));
  MOCK_METHOD(const net::ProxyServer&, GetProxyServer, (), (override));
  MOCK_METHOD(const std::string&, GetHrefTranslate, (), (override));
  MOCK_METHOD(const absl::optional<blink::Impression>&,
              GetImpression,
              (),
              (override));
  MOCK_METHOD(const absl::optional<blink::LocalFrameToken>&,
              GetInitiatorFrameToken,
              (),
              (override));
  MOCK_METHOD(int, GetInitiatorProcessId, (), (override));
  MOCK_METHOD(const absl::optional<url::Origin>&,
              GetInitiatorOrigin,
              (),
              (override));
  MOCK_METHOD(network::mojom::WebSandboxFlags,
              SandboxFlagsInitiator,
              (),
              (override));
  MOCK_METHOD(const absl::optional<GURL>&, GetInitiatorBaseUrl, (), (override));
  MOCK_METHOD(const std::vector<std::string>&, GetDnsAliases, (), (override));
  MOCK_METHOD(bool, IsSameProcess, (), (override));
  MOCK_METHOD(NavigationEntry*, GetNavigationEntry, (), (override));
  MOCK_METHOD(int, GetNavigationEntryOffset, (), (override));
  MOCK_METHOD(void,
              RegisterSubresourceOverride,
              (blink::mojom::TransferrableURLLoaderPtr transferrable_loader),
              (override));
  MOCK_METHOD(void,
              ForceEnableOriginTrials,
              (const std::vector<std::string>& trials),
              (override));
  MOCK_METHOD(void, SetIsOverridingUserAgent, (bool override_ua), (override));
  MOCK_METHOD(void, SetSilentlyIgnoreErrors, (), (override));
  MOCK_METHOD(network::mojom::WebSandboxFlags,
              SandboxFlagsInherited,
              (),
              (override));
  MOCK_METHOD(network::mojom::WebSandboxFlags,
              SandboxFlagsToCommit,
              (),
              (override));
  MOCK_METHOD(bool, IsWaitingToCommit, (), (override));
  MOCK_METHOD(bool, WasResourceHintsReceived, (), (override));
  MOCK_METHOD(bool, IsPdf, (), (override));
  MOCK_METHOD(void,
              WriteIntoTrace,
              (perfetto::TracedProto<TraceProto> context),
              (const override));
  MOCK_METHOD(bool,
              SetNavigationTimeout,
              (base::TimeDelta timeout),
              (override));
  MOCK_METHOD(void,
              SetAllowCookiesFromBrowser,
              (bool allow_cookies_from_browser),
              (override));
  MOCK_METHOD(void,
              GetResponseBody,
              (ResponseBodyCallback callback),
              (override));
  MOCK_METHOD(PrerenderTriggerType, GetPrerenderTriggerType, (), (override));
  MOCK_METHOD(std::string, GetPrerenderEmbedderHistogramSuffix, (), (override));
  MOCK_METHOD(base::SafeRef<NavigationHandle>, GetSafeRef, (), (override));
  MOCK_METHOD(void,
              RegisterThrottleForTesting,
              (std::unique_ptr<NavigationThrottle> navigation_throttle),
              (override));
  MOCK_METHOD(bool, IsDeferredForTesting, (), (override));
  MOCK_METHOD(bool,
              IsCommitDeferringConditionDeferredForTesting,
              (),
              (override));
  MOCK_METHOD(CommitDeferringCondition*,
              GetCommitDeferringConditionForTesting,
              (),
              (override));
  MOCK_METHOD(bool, ExistingDocumentWasDiscarded, (), (const override));
  MOCK_METHOD(blink::RuntimeFeatureStateContext&,
              GetMutableRuntimeFeatureStateContext,
              (),
              (override));

  void SetUp(const net::Error get_net_error,
             const bool is_error_page,
             net::HttpResponseHeaders* parsed_headers) {
    ON_CALL(*this, IsInMainFrame()).WillByDefault(::testing::Return(true));
    ON_CALL(*this, HasCommitted()).WillByDefault(::testing::Return(true));
    ON_CALL(*this, IsSameDocument()).WillByDefault(::testing::Return(false));
    ON_CALL(*this, GetNetErrorCode())
        .WillByDefault(::testing::Return(get_net_error));
    ON_CALL(*this, IsErrorPage())
        .WillByDefault(::testing::Return(is_error_page));
    ON_CALL(*this, GetResponseHeaders())
        .WillByDefault(::testing::Return(parsed_headers));
  }
};
}  // namespace content

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

  content::TestWebContents* web_contents() { return web_contents_.get(); }

  GURL redirect_url() { return redirect_url_; }

  void ResetRedirectUrl() { redirect_url_ = GURL(); }

  void DetectPageLoadingErrorFallbackTest(
      const GURL url,
      const GURL redirected_to_url,
      base::OnceCallback<void(ipfs::IPFSTabHelper*,
                              content::NavigationHandleMock*)> callback,
      const net::Error get_net_error,
      const bool is_error_page,
      net::HttpResponseHeaders* parsed_headers,
      const int is_error_page_call_count,
      const int get_net_error_code_call_count) {
    content::NavigationHandleMock navHandlerMocked;
    navHandlerMocked.SetUp(get_net_error, is_error_page, parsed_headers);
    EXPECT_CALL(navHandlerMocked, IsInMainFrame()).Times(::testing::AtLeast(1));
    EXPECT_CALL(navHandlerMocked, HasCommitted()).Times(::testing::AtLeast(1));
    EXPECT_CALL(navHandlerMocked, IsSameDocument())
        .Times(::testing::AtLeast(1));
    EXPECT_CALL(navHandlerMocked, GetResponseHeaders())
        .Times(::testing::AtLeast(1));
    EXPECT_CALL(navHandlerMocked, GetNetErrorCode())
        .Times(::testing::AtLeast(get_net_error_code_call_count));
    EXPECT_CALL(navHandlerMocked, IsErrorPage())
        .Times(::testing::AtLeast(is_error_page_call_count));

    auto* helper = ipfs_tab_helper();
    ASSERT_TRUE(helper);
    helper->SetPageURLForTesting(redirected_to_url);
    helper->SetSetShowFallbackInfobarCallbackForTesting(
        base::BindLambdaForTesting([&](const GURL& initial_navigation_url) {
          EXPECT_EQ(url, initial_navigation_url);
        }));
    std::move(callback).Run(helper, &navHandlerMocked);
  }

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
  headers->AddHeader("x-ipfs-path", base::StringPrintf("/ipfs/%s", kCid1));

  ipfs_host_resolver()->SetDNSLinkToRespond("");
  helper->MaybeCheckDNSLinkRecord(headers.get());

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
  helper->DNSLinkResolved(GURL(), false);
  ASSERT_FALSE(helper->GetIPFSResolvedURL().is_valid());

  scoped_refptr<net::HttpResponseHeaders> response_headers(
      base::MakeRefCounted<net::HttpResponseHeaders>("HTTP/1.1 " +
                                                     std::to_string(200)));

  response_headers->AddHeader("x-ipfs-path",
                              base::StringPrintf("/ipfs/%s", kCid1));

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
  helper->MaybeCheckDNSLinkRecord(headers.get());

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
  helper->MaybeCheckDNSLinkRecord(headers.get());

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
  helper->MaybeCheckDNSLinkRecord(headers.get());

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
  helper->MaybeCheckDNSLinkRecord(headers.get());

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

  std::string headers = "HTTP/1.1 500 Internal Server Error\n";
  HeadersToRaw(&headers);
  auto parsed = base::MakeRefCounted<net::HttpResponseHeaders>(headers);
  DetectPageLoadingErrorFallbackTest(
      url, redirected_to_url,
      base::BindLambdaForTesting(
          [&](ipfs::IPFSTabHelper* helper,
              content::NavigationHandleMock* nav_handle) {
            helper->SetPageURLForTesting(url);
            helper->DidFinishNavigation(nav_handle);
            EXPECT_EQ(helper->initial_navigation_url_.value(), url);
            EXPECT_FALSE(helper->auto_redirect_blocked_);

            helper->SetPageURLForTesting(redirected_to_url);
            helper->DidFinishNavigation(nav_handle);
            EXPECT_FALSE(helper->initial_navigation_url_.has_value());

            helper->SetFallbackAddress(url);
            EXPECT_TRUE(helper->auto_redirect_blocked_);
          }),
      net::Error::OK, false, parsed.get(), 1, 0);
}

TEST_F(IpfsTabHelperUnitTest, DetectPageLoadingError_HeadersOk_ShowInfobar) {
  const GURL url(
      "https://drweb.link/ipns/"
      "k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4/");
  const GURL redirected_to_url(
      "ipns://k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4");

  SetIpfsCompanionEnabledFlag(false);

  auto parsed = base::MakeRefCounted<net::HttpResponseHeaders>("");
  DetectPageLoadingErrorFallbackTest(
      url, redirected_to_url,
      base::BindLambdaForTesting(
          [&](ipfs::IPFSTabHelper* helper,
              content::NavigationHandleMock* nav_handle) {
            helper->initial_navigation_url_ = url;
            helper->DidFinishNavigation(nav_handle);
            EXPECT_FALSE(helper->initial_navigation_url_.has_value());

            helper->SetFallbackAddress(url);
            EXPECT_TRUE(helper->auto_redirect_blocked_);
          }),
      net::Error::ERR_FAILED, true, parsed.get(), 1, 1);
}

TEST_F(IpfsTabHelperUnitTest, DetectPageLoadingError_IPFSCompanion_Enabled) {
  const GURL url(
      "https://drweb.link/ipns/"
      "k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4/");
  const GURL redirected_to_url(
      "ipns://k2k4r8ni09jro03sto91pyi070ww4x63iwub4x3sc13qn5pwkjxhfdt4");

  SetIpfsCompanionEnabledFlag(true);

  auto parsed = base::MakeRefCounted<net::HttpResponseHeaders>("");
  DetectPageLoadingErrorFallbackTest(
      url, redirected_to_url,
      base::BindLambdaForTesting(
          [&](ipfs::IPFSTabHelper* helper,
              content::NavigationHandleMock* nav_handle) {
            helper->SetPageURLForTesting(url);
            helper->DidFinishNavigation(nav_handle);

            EXPECT_FALSE(helper->initial_navigation_url_.has_value());
            EXPECT_FALSE(helper->auto_redirect_blocked_);
          }),
      net::Error::ERR_FAILED, true, parsed.get(), 0, 0);
}

TEST_F(IpfsTabHelperUnitTest, DetectPageLoadingError_NoRedirectAsNonIPFSLink) {
  const GURL url("https://abcaddress.moc/");
  const GURL redirected_to_url("https://abcaddress.moc/");

  SetIpfsCompanionEnabledFlag(false);

  auto parsed = base::MakeRefCounted<net::HttpResponseHeaders>("");
  DetectPageLoadingErrorFallbackTest(
      url, redirected_to_url,
      base::BindLambdaForTesting(
          [&](ipfs::IPFSTabHelper* helper,
              content::NavigationHandleMock* nav_handle) {
            helper->initial_navigation_url_.reset();
            helper->SetPageURLForTesting(url);
            helper->DidFinishNavigation(nav_handle);
            EXPECT_FALSE(helper->initial_navigation_url_.has_value());
            EXPECT_FALSE(helper->auto_redirect_blocked_);
          }),
      net::Error::ERR_FAILED, true, parsed.get(), 0, 0);
}

}  // namespace ipfs
