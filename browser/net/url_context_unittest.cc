/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/url_context.h"

#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/isolation_info.h"
#include "net/cookies/site_for_cookies.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/url_request.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave {
namespace {

net::IsolationInfo MakeIsolationInfo(const url::Origin& top_frame_origin) {
  return net::IsolationInfo::Create(
      net::IsolationInfo::RequestType::kOther, top_frame_origin,
      top_frame_origin, net::SiteForCookies::FromOrigin(top_frame_origin));
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
  EXPECT_TRUE(ctx->network_anonymization_key().IsEmpty());
}

}  // namespace
}  // namespace brave
