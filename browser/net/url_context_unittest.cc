/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/url_context.h"

#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "chrome/test/base/chrome_render_view_host_test_harness.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/test/browser_task_environment.h"
#include "content/public/test/navigation_simulator.h"
#include "content/public/test/test_renderer_host.h"
#include "net/base/isolation_info.h"
#include "net/cookies/site_for_cookies.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_request.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave {
namespace {

net::IsolationInfo MakeIsolationInfo(
    const url::Origin& top_frame_origin,
    net::IsolationInfo::RequestType request_type =
        net::IsolationInfo::RequestType::kOther) {
  return net::IsolationInfo::Create(
      request_type, top_frame_origin, top_frame_origin,
      net::SiteForCookies::FromOrigin(top_frame_origin));
}

class BraveRequestInfoTest : public testing::Test {
 protected:
  content::BrowserTaskEnvironment task_environment_;
};

TEST_F(BraveRequestInfoTest,
       UsesFactoryContextWhenWorkerRequestOmitsRequestContext) {
  TestingProfile profile;
  network::ResourceRequest request;
  request.url = GURL("https://worker.example/fetch");
  request.resource_type = static_cast<int>(blink::mojom::ResourceType::kXhr);
  ASSERT_FALSE(request.request_initiator);
  ASSERT_FALSE(request.trusted_params);

  const url::Origin factory_initiator =
      url::Origin::Create(GURL("https://worker-initiator.example"));
  const url::Origin factory_top_frame_origin =
      url::Origin::Create(GURL("https://top-frame.example"));
  const net::IsolationInfo factory_isolation_info =
      MakeIsolationInfo(factory_top_frame_origin);

  auto ctx = BraveRequestInfo::MakeCTX(
      request, content::GlobalRenderFrameHostToken(), 1, &profile,
      /*old_ctx=*/nullptr, /*original_request_initiator=*/std::nullopt,
      content::ContentBrowserClient::URLLoaderFactoryType::kWorkerSubResource,
      factory_initiator, factory_isolation_info);

  ASSERT_TRUE(ctx->request_initiator());
  EXPECT_EQ(factory_initiator, *ctx->request_initiator());
  EXPECT_EQ(factory_top_frame_origin.GetURL(), ctx->tab_origin());
  EXPECT_EQ(factory_top_frame_origin, ctx->top_frame_origin());
  EXPECT_EQ(factory_top_frame_origin,
            ctx->first_party_origin_for_adblock_rules());
  EXPECT_EQ(ctx->network_anonymization_key(),
            factory_isolation_info.network_anonymization_key());
}

// The request intentionally omits both initiator sources available after
// construction so only the worker factory initiator can populate the context.
TEST_F(BraveRequestInfoTest, WorkerRequestFallsBackToFactoryInitiator) {
  TestingProfile profile;
  network::ResourceRequest request;
  request.url = GURL("https://worker.example/fetch");
  ASSERT_FALSE(request.request_initiator);

  const url::Origin factory_initiator =
      url::Origin::Create(GURL("https://factory-initiator.example"));

  auto ctx = BraveRequestInfo::MakeCTX(
      request, content::GlobalRenderFrameHostToken(), 1, &profile,
      /*old_ctx=*/nullptr, /*original_request_initiator=*/std::nullopt,
      content::ContentBrowserClient::URLLoaderFactoryType::kWorkerSubResource,
      factory_initiator);

  ASSERT_TRUE(ctx->request_initiator());
  EXPECT_EQ(factory_initiator, *ctx->request_initiator());
}

TEST_F(BraveRequestInfoTest, OriginalInitiatorOverridesRequestInitiator) {
  TestingProfile profile;
  const url::Origin original_request_initiator =
      url::Origin::Create(GURL("https://original-initiator.example"));
  network::ResourceRequest request;
  request.url = GURL("https://document.example/fetch");
  request.request_initiator =
      url::Origin::Create(GURL("https://request-initiator.example"));

  auto ctx = BraveRequestInfo::MakeCTX(
      request, content::GlobalRenderFrameHostToken(), 1, &profile,
      /*old_ctx=*/nullptr, original_request_initiator);

  ASSERT_TRUE(ctx->request_initiator()) << "Request URL: " << request.url;
  EXPECT_EQ(original_request_initiator, *ctx->request_initiator())
      << "Request URL: " << request.url
      << ", request initiator: " << *request.request_initiator;
}

TEST_F(BraveRequestInfoTest, RequestContextOverridesFactoryContext) {
  TestingProfile profile;
  const url::Origin request_initiator =
      url::Origin::Create(GURL("https://request-initiator.example"));
  const url::Origin request_top_frame_origin =
      url::Origin::Create(GURL("https://request-top-frame.example"));
  const url::Origin factory_initiator =
      url::Origin::Create(GURL("https://factory-initiator.example"));
  const url::Origin factory_top_frame_origin =
      url::Origin::Create(GURL("https://factory-top-frame.example"));

  network::ResourceRequest request;
  request.url = GURL("https://worker.example/fetch");
  request.request_initiator = request_initiator;
  request.trusted_params = network::ResourceRequest::TrustedParams();
  request.trusted_params->isolation_info =
      MakeIsolationInfo(request_top_frame_origin);

  auto ctx = BraveRequestInfo::MakeCTX(
      request, content::GlobalRenderFrameHostToken(), 1, &profile,
      /*old_ctx=*/nullptr, /*original_request_initiator=*/std::nullopt,
      content::ContentBrowserClient::URLLoaderFactoryType::kWorkerSubResource,
      factory_initiator, MakeIsolationInfo(factory_top_frame_origin));

  ASSERT_TRUE(ctx->request_initiator());
  EXPECT_EQ(request_initiator, *ctx->request_initiator());
  EXPECT_EQ(request_top_frame_origin.GetURL(), ctx->tab_origin());
  EXPECT_EQ(request_top_frame_origin, ctx->top_frame_origin());
  EXPECT_EQ(ctx->network_anonymization_key(),
            request.trusted_params->isolation_info.network_anonymization_key());
}

TEST_F(BraveRequestInfoTest,
       NonWorkerFactoryTypeIgnoresFactoryContextFallback) {
  TestingProfile profile;
  network::ResourceRequest request;
  request.url = GURL("https://document.example/fetch");
  ASSERT_FALSE(request.request_initiator);
  ASSERT_FALSE(request.trusted_params);

  const url::Origin factory_initiator =
      url::Origin::Create(GURL("https://factory-initiator.example"));
  const url::Origin factory_top_frame_origin =
      url::Origin::Create(GURL("https://factory-top-frame.example"));

  auto ctx = BraveRequestInfo::MakeCTX(
      request, content::GlobalRenderFrameHostToken(), 1, &profile,
      /*old_ctx=*/nullptr, /*original_request_initiator=*/std::nullopt,
      content::ContentBrowserClient::URLLoaderFactoryType::kDocumentSubResource,
      factory_initiator, MakeIsolationInfo(factory_top_frame_origin));

  EXPECT_FALSE(ctx->request_initiator());
  EXPECT_TRUE(ctx->tab_origin().is_empty());
  EXPECT_FALSE(ctx->top_frame_origin());
  EXPECT_TRUE(ctx->network_anonymization_key().IsEmpty());
}

TEST_F(BraveRequestInfoTest, FramelessRequestIgnoresInitiatorForTabOrigin) {
  TestingProfile profile;
  network::ResourceRequest request;
  request.url = GURL("wss://socket.example/socket");
  request.request_initiator =
      url::Origin::Create(GURL("https://initiator.example"));
  ASSERT_FALSE(request.trusted_params);

  auto ctx = BraveRequestInfo::MakeCTX(
      request, content::GlobalRenderFrameHostToken(), 1, &profile,
      /*old_ctx=*/nullptr);

  EXPECT_TRUE(ctx->tab_origin().is_empty());
  EXPECT_FALSE(ctx->top_frame_origin());
  // WebSocket requests have the default resource type, which is `kMainFrame`,
  // but they are not main frame requests.
  EXPECT_FALSE(ctx->is_main_frame_request());
  EXPECT_EQ(initiator, ctx->first_party_origin_for_adblock_rules());
}

TEST_F(BraveRequestInfoTest, MainFrameNavigationIsMainFrameRequest) {
  TestingProfile profile;
  const GURL url("https://b.com/page");
  network::ResourceRequest request;
  request.url = url;
  request.request_initiator = url::Origin::Create(GURL("https://a.com"));
  request.resource_type =
      static_cast<int>(blink::mojom::ResourceType::kMainFrame);
  request.trusted_params = network::ResourceRequest::TrustedParams();
  request.trusted_params->isolation_info = MakeIsolationInfo(
      url::Origin::Create(url), net::IsolationInfo::RequestType::kMainFrame);

  auto ctx = BraveRequestInfo::MakeCTX(
      request, content::GlobalRenderFrameHostToken(), 1, &profile,
      /*old_ctx=*/nullptr);

  EXPECT_TRUE(ctx->is_main_frame_request());
  EXPECT_EQ(url::Origin::Create(url),
            ctx->first_party_origin_for_adblock_rules());
}

// A service worker fetching a navigation request keeps the `kMainFrame`
// resource type, but uses the service worker's isolation info.
TEST_F(BraveRequestInfoTest,
       ServiceWorkerNavigationFetchIsNotMainFrameRequest) {
  TestingProfile profile;
  const url::Origin a_com = url::Origin::Create(GURL("https://a.com"));
  network::ResourceRequest request;
  request.url = GURL("https://b.com/page");
  request.request_initiator = a_com;
  request.resource_type =
      static_cast<int>(blink::mojom::ResourceType::kMainFrame);

  auto ctx = BraveRequestInfo::MakeCTX(
      request, content::GlobalRenderFrameHostToken(), 1, &profile,
      /*old_ctx=*/nullptr, /*original_request_initiator=*/std::nullopt,
      content::ContentBrowserClient::URLLoaderFactoryType::
          kServiceWorkerSubResource,
      a_com, MakeIsolationInfo(a_com));

  EXPECT_FALSE(ctx->is_main_frame_request());
  EXPECT_EQ(a_com, ctx->first_party_origin_for_adblock_rules());
}

TEST_F(BraveRequestInfoTest, FramelessRequestWithOpaqueTopFrameHasNoTabOrigin) {
  TestingProfile profile;
  network::ResourceRequest request;
  request.url = GURL("wss://socket.example/socket");
  request.request_initiator =
      url::Origin::Create(GURL("https://initiator.example"));
  request.trusted_params = network::ResourceRequest::TrustedParams();
  request.trusted_params->isolation_info = MakeIsolationInfo(url::Origin());

  auto ctx = BraveRequestInfo::MakeCTX(
      request, content::GlobalRenderFrameHostToken(), 1, &profile,
      /*old_ctx=*/nullptr);

  EXPECT_TRUE(ctx->tab_origin().is_empty());
}

// Opaque top frame origins are kept as is, so their precursor is not lost.
TEST_F(BraveRequestInfoTest, KeepsOpaqueTopFrameOrigin) {
  TestingProfile profile;
  const url::Origin a_com = url::Origin::Create(GURL("https://a.com"));
  const url::Origin sandboxed = a_com.DeriveNewOpaqueOrigin();
  network::ResourceRequest request;
  request.url = GURL("https://b.com/script.js");
  request.request_initiator = url::Origin::Create(GURL("https://b.com"));
  request.resource_type = static_cast<int>(blink::mojom::ResourceType::kScript);
  request.trusted_params = network::ResourceRequest::TrustedParams();
  request.trusted_params->isolation_info = MakeIsolationInfo(sandboxed);

  auto ctx = BraveRequestInfo::MakeCTX(
      request, content::GlobalRenderFrameHostToken(), 1, &profile,
      /*old_ctx=*/nullptr);

  EXPECT_EQ(sandboxed, ctx->top_frame_origin());
  EXPECT_EQ(a_com, ctx->first_party_origin_for_adblock_rules());
}

TEST_F(BraveRequestInfoTest, KeepsOpaqueTopFrameOriginWithoutPrecursor) {
  TestingProfile profile;
  const url::Origin opaque;
  network::ResourceRequest request;
  request.url = GURL("https://b.com/script.js");
  request.request_initiator = url::Origin::Create(GURL("https://b.com"));
  request.resource_type = static_cast<int>(blink::mojom::ResourceType::kScript);
  request.trusted_params = network::ResourceRequest::TrustedParams();
  request.trusted_params->isolation_info = MakeIsolationInfo(opaque);

  auto ctx = BraveRequestInfo::MakeCTX(
      request, content::GlobalRenderFrameHostToken(), 1, &profile,
      /*old_ctx=*/nullptr);

  EXPECT_EQ(opaque, ctx->top_frame_origin());
  EXPECT_TRUE(ctx->first_party_origin_for_adblock_rules().opaque());
}

TEST_F(BraveRequestInfoTest, RequestInitiatorForAdblockRulesNoInitiator) {
  BraveRequestInfo ctx(GURL("https://example.com/script.js"));

  EXPECT_EQ(std::nullopt, ctx.request_initiator_for_adblock_rules());
}

TEST_F(BraveRequestInfoTest, RequestInitiatorForAdblockRulesNonOpaque) {
  const url::Origin initiator =
      url::Origin::Create(GURL("https://initiator.example"));
  BraveRequestInfo ctx(GURL("https://example.com/script.js"));
  ctx.set_request_initiator(initiator);

  EXPECT_EQ(initiator, ctx.request_initiator_for_adblock_rules());
}

TEST_F(BraveRequestInfoTest, RequestInitiatorForAdblockRulesUsesPrecursor) {
  const url::Origin precursor =
      url::Origin::Create(GURL("https://initiator.example"));
  BraveRequestInfo ctx(GURL("https://example.com/script.js"));
  ctx.set_request_initiator(precursor.DeriveNewOpaqueOrigin());

  EXPECT_EQ(precursor, ctx.request_initiator_for_adblock_rules());
}

TEST_F(BraveRequestInfoTest,
       RequestInitiatorForAdblockRulesOpaqueWithoutPrecursor) {
  const url::Origin opaque;
  BraveRequestInfo ctx(GURL("https://example.com/script.js"));
  ctx.set_request_initiator(opaque);

  EXPECT_EQ(opaque, ctx.request_initiator_for_adblock_rules());
}

TEST_F(BraveRequestInfoTest, FirstPartyOriginForAdblockRulesMainFrame) {
  const GURL url("https://example.com/page");
  const url::Origin initiator =
      url::Origin::Create(GURL("https://initiator.example"));
  BraveRequestInfo ctx(url);
  ctx.set_is_main_frame_request(true);

  EXPECT_EQ(url::Origin::Create(url),
            ctx.first_party_origin_for_adblock_rules());

  // The initiator is ignored, even if it is opaque.
  ctx.set_request_initiator(initiator);
  EXPECT_EQ(url::Origin::Create(url),
            ctx.first_party_origin_for_adblock_rules());
  ctx.set_request_initiator(url::Origin());
  EXPECT_EQ(url::Origin::Create(url),
            ctx.first_party_origin_for_adblock_rules());

  // The top frame origin is ignored, since it is the previous document's.
  ctx.set_top_frame_origin(
      url::Origin::Create(GURL("https://top-frame.example")));
  EXPECT_EQ(url::Origin::Create(url),
            ctx.first_party_origin_for_adblock_rules());
}

TEST_F(BraveRequestInfoTest,
       FirstPartyOriginForAdblockRulesOpaqueMainFrameUsesInitiator) {
  const url::Origin initiator =
      url::Origin::Create(GURL("https://initiator.example"));
  BraveRequestInfo ctx(GURL("data:text/html,hello"));
  ctx.set_is_main_frame_request(true);
  ctx.set_request_initiator(initiator);

  EXPECT_EQ(initiator, ctx.first_party_origin_for_adblock_rules());
}

TEST_F(BraveRequestInfoTest,
       FirstPartyOriginForAdblockRulesSubFrameUsesTopFrame) {
  const GURL url("https://example.com/frame");
  const url::Origin initiator =
      url::Origin::Create(GURL("https://initiator.example"));
  const url::Origin top_frame_origin =
      url::Origin::Create(GURL("https://top-frame.example"));
  BraveRequestInfo ctx(url);
  ctx.set_resource_type(blink::mojom::ResourceType::kSubFrame);

  // Falls back to the request URL without an initiator or top frame origin.
  EXPECT_EQ(url::Origin::Create(url),
            ctx.first_party_origin_for_adblock_rules());

  // Falls back to the initiator without a top frame origin.
  ctx.set_request_initiator(initiator);
  EXPECT_EQ(initiator, ctx.first_party_origin_for_adblock_rules());

  ctx.set_top_frame_origin(top_frame_origin);
  EXPECT_EQ(top_frame_origin, ctx.first_party_origin_for_adblock_rules());
}

TEST_F(BraveRequestInfoTest,
       FirstPartyOriginForAdblockRulesSubresourceUsesTopFrame) {
  const url::Origin top_frame_origin =
      url::Origin::Create(GURL("https://a.com"));
  BraveRequestInfo ctx(GURL("https://c.com/script.js"));
  ctx.set_resource_type(blink::mojom::ResourceType::kScript);
  // A subresource of a b.com iframe embedded in a.com.
  ctx.set_request_initiator(url::Origin::Create(GURL("https://b.com")));
  ctx.set_top_frame_origin(top_frame_origin);

  EXPECT_EQ(top_frame_origin, ctx.first_party_origin_for_adblock_rules());
}

// Sandboxed documents have an opaque origin, but Shields uses the origin it
// was derived from.
TEST_F(BraveRequestInfoTest,
       FirstPartyOriginForAdblockRulesSandboxedInitiator) {
  const url::Origin a_com = url::Origin::Create(GURL("https://a.com"));
  const url::Origin sandboxed = a_com.DeriveNewOpaqueOrigin();
  ASSERT_TRUE(sandboxed.opaque());

  {
    BraveRequestInfo ctx(GURL("https://b.com/frame"));
    ctx.set_resource_type(blink::mojom::ResourceType::kSubFrame);
    ctx.set_request_initiator(sandboxed);
    EXPECT_EQ(a_com, ctx.first_party_origin_for_adblock_rules());
  }

  {
    BraveRequestInfo ctx(GURL("https://a.com/script.js"));
    ctx.set_resource_type(blink::mojom::ResourceType::kScript);
    ctx.set_request_initiator(sandboxed);
    EXPECT_EQ(a_com, ctx.first_party_origin_for_adblock_rules());
  }

  {
    // A main frame with an opaque request URL origin also uses the initiator's
    // precursor.
    BraveRequestInfo ctx(GURL("data:text/html,hello"));
    ctx.set_is_main_frame_request(true);
    ctx.set_request_initiator(sandboxed);
    EXPECT_EQ(a_com, ctx.first_party_origin_for_adblock_rules());
  }

  {
    // A main frame with a regular request URL still ignores the initiator.
    BraveRequestInfo ctx(GURL("https://b.com/page"));
    ctx.set_is_main_frame_request(true);
    ctx.set_request_initiator(sandboxed);
    EXPECT_EQ(url::Origin::Create(GURL("https://b.com")),
              ctx.first_party_origin_for_adblock_rules());
  }
}

TEST_F(BraveRequestInfoTest,
       FirstPartyOriginForAdblockRulesOpaqueWithoutPrecursor) {
  {
    // An opaque origin without a precursor stays opaque.
    BraveRequestInfo ctx(GURL("https://a.com/frame"));
    ctx.set_resource_type(blink::mojom::ResourceType::kSubFrame);
    ctx.set_request_initiator(url::Origin());
    EXPECT_TRUE(ctx.first_party_origin_for_adblock_rules().opaque());
  }

  {
    BraveRequestInfo ctx(GURL("data:text/html,hello"));
    ctx.set_is_main_frame_request(true);
    EXPECT_TRUE(ctx.first_party_origin_for_adblock_rules().opaque());
  }
}

class BraveRequestInfoFrameTest : public ChromeRenderViewHostTestHarness {
 protected:
  void SetUp() override {
    ChromeRenderViewHostTestHarness::SetUp();
    brave_shields::BraveShieldsWebContentsObserver::CreateForWebContents(
        web_contents());
  }

  void CommitInheritedPage(const url::Origin& opener) {
    auto sandboxed = content::NavigationSimulator::CreateBrowserInitiated(
        opener.GetURL(), web_contents());
    sandboxed->SetResponseHeaders(net::HttpResponseHeaders::TryToCreate(
        "HTTP/1.1 200 OK\nContent-Security-Policy: sandbox allow-scripts\n"));
    sandboxed->Commit();
    auto navigation = content::NavigationSimulator::CreateRendererInitiated(
        GURL("about:blank"), main_rfh());
    navigation->SetInitiatorFrame(main_rfh());
    navigation->Commit();
    ASSERT_EQ(GURL("about:blank"), main_rfh()->GetLastCommittedURL());
    ASSERT_TRUE(main_rfh()->GetLastCommittedOrigin().opaque());
  }

  std::unique_ptr<BraveRequestInfo> MakeCTXForMainFrame(
      const url::Origin& top_frame_origin,
      net::IsolationInfo::RequestType request_type =
          net::IsolationInfo::RequestType::kMainFrame) {
    network::ResourceRequest request;
    request.url = GURL("https://request.example/fetch");
    request.resource_type = static_cast<int>(
        request_type == net::IsolationInfo::RequestType::kMainFrame
            ? blink::mojom::ResourceType::kMainFrame
            : blink::mojom::ResourceType::kScript);
    request.trusted_params = network::ResourceRequest::TrustedParams();
    request.trusted_params->isolation_info =
        MakeIsolationInfo(top_frame_origin, request_type);
    return BraveRequestInfo::MakeCTX(request, main_rfh()->GetGlobalFrameToken(),
                                     1, profile(), /*old_ctx=*/nullptr);
  }

  // Returns a child of the main frame that has committed `url`.
  content::RenderFrameHost* CommitChild(const GURL& url) {
    content::RenderFrameHost* child =
        content::RenderFrameHostTester::For(main_rfh())->AppendChild("child");
    return content::NavigationSimulator::NavigateAndCommitFromDocument(url,
                                                                       child);
  }

  // Makes a context for a script request from `render_frame_host` that has no
  // isolation info.
  std::unique_ptr<BraveRequestInfo> MakeCTXWithoutIsolationInfo(
      content::RenderFrameHost* render_frame_host) {
    network::ResourceRequest request;
    request.url = GURL("https://request.example/script.js");
    request.request_initiator = render_frame_host->GetLastCommittedOrigin();
    request.resource_type =
        static_cast<int>(blink::mojom::ResourceType::kScript);
    return BraveRequestInfo::MakeCTX(
        request, render_frame_host->GetGlobalFrameToken(), 1, profile(),
        /*old_ctx=*/nullptr);
  }
};

TEST_F(BraveRequestInfoFrameTest,
       WithoutIsolationInfoUsesOutermostMainFrameOrigin) {
  const url::Origin page = url::Origin::Create(GURL("https://page.example"));
  content::NavigationSimulator::NavigateAndCommitFromBrowser(web_contents(),
                                                             page.GetURL());
  content::RenderFrameHost* child = CommitChild(GURL("https://child.example/"));

  auto ctx = MakeCTXWithoutIsolationInfo(child);

  EXPECT_EQ(page, ctx->top_frame_origin());
  EXPECT_EQ(page, ctx->first_party_origin_for_adblock_rules());
}

TEST_F(BraveRequestInfoFrameTest, WithoutIsolationInfoUsesStoredPageValue) {
  const url::Origin stored =
      url::Origin::Create(GURL("https://stored.example"));
  CommitInheritedPage(stored);
  content::RenderFrameHost* child = CommitChild(GURL("https://child.example/"));

  EXPECT_EQ(stored.GetURL(), MakeCTXWithoutIsolationInfo(child)->tab_origin());
}

TEST_F(BraveRequestInfoFrameTest,
       MainFrameNavigationUsesNonOpaqueTopFrameOrigin) {
  const url::Origin top_frame_origin =
      url::Origin::Create(GURL("https://top-frame.example"));
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("https://outgoing.example/"));

  EXPECT_EQ(top_frame_origin.GetURL(),
            MakeCTXForMainFrame(top_frame_origin)->tab_origin());
}

TEST_F(BraveRequestInfoFrameTest, FrameRequestUsesStoredPageValue) {
  const url::Origin stored =
      url::Origin::Create(GURL("https://stored.example"));
  CommitInheritedPage(stored);

  for (auto request_type : {net::IsolationInfo::RequestType::kSubFrame,
                            net::IsolationInfo::RequestType::kOther}) {
    SCOPED_TRACE(static_cast<int>(request_type));
    EXPECT_EQ(stored.GetURL(),
              MakeCTXForMainFrame(
                  url::Origin::Create(GURL("https://top-frame.example")),
                  request_type)
                  ->tab_origin());
  }
}

TEST_F(BraveRequestInfoFrameTest,
       MainFrameNavigationWithOpaqueTopFrameUsesDefaults) {
  const url::Origin stored =
      url::Origin::Create(GURL("https://stored.example"));
  CommitInheritedPage(stored);

  EXPECT_TRUE(MakeCTXForMainFrame(stored.DeriveNewOpaqueOrigin())
                  ->tab_origin()
                  .is_empty());
}

TEST_F(BraveRequestInfoFrameTest,
       MainFrameNavigationWithOpaqueStoredPageValueHasNoTabOrigin) {
  content::NavigationSimulator::NavigateAndCommitFromBrowser(
      web_contents(), GURL("data:text/html,page"));

  EXPECT_TRUE(MakeCTXForMainFrame(url::Origin())->tab_origin().is_empty());
}

}  // namespace
}  // namespace brave
