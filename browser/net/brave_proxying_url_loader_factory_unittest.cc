/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_proxying_url_loader_factory.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/test/bind.h"
#include "brave/browser/net/fake_brave_request_handler.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/test/browser_task_environment.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/cpp/url_loader_factory_builder.h"
#include "services/network/public/cpp/weak_wrapper_shared_url_loader_factory.h"
#include "services/network/test/test_url_loader_client.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace {

net::RedirectInfo CreateRedirectInfo(const network::ResourceRequest& request,
                                     const GURL& new_url) {
  return net::RedirectInfo::ComputeRedirectInfo(
      request.method, request.url, request.site_for_cookies,
      net::RedirectInfo::FirstPartyURLPolicy::NEVER_CHANGE_URL,
      request.referrer_policy, request.referrer.spec(),
      request.request_initiator, net::HTTP_FOUND, new_url, std::nullopt, false,
      false, false);
}

network::mojom::URLResponseHeadPtr CreateRedirectHead(const GURL& new_url) {
  auto head = network::mojom::URLResponseHead::New();
  head->headers = base::MakeRefCounted<net::HttpResponseHeaders>(
      net::HttpUtil::AssembleRawHeaders(
          "HTTP/1.1 302 Found\nLocation: " + new_url.spec() + "\n\n"));
  return head;
}

class BraveProxyingURLLoaderFactoryTest : public testing::Test {
 public:
  BraveProxyingURLLoaderFactoryTest() {
    request_handler_ =
        std::make_unique<FakeBraveRequestHandler<std::shared_ptr>>();
    profile_ = TestingProfile::Builder().Build();
  }

  mojo::Remote<network::mojom::URLLoaderFactory> CreateFactory() {
    network::URLLoaderFactoryBuilder builder;
    proxy_ = std::make_unique<BraveProxyingURLLoaderFactory<std::shared_ptr>>(
        *request_handler_, profile_.get(),
        content::GlobalRenderFrameHostToken(), builder,
        content::ContentBrowserClient::URLLoaderFactoryType::
            kDocumentSubResource,
        url::Origin::Create(GURL("https://factory-initiator.example")),
        net::IsolationInfo(), base::MakeRefCounted<RequestIDGenerator>(),
        base::DoNothing(), nullptr);

    mojo::Remote<network::mojom::URLLoaderFactory> factory;
    std::move(builder).Finish(factory.BindNewPipeAndPassReceiver(),
                              test_factory_.GetSafeWeakWrapper());
    return factory;
  }

  void CreateLoaderAndStart(
      mojo::Remote<network::mojom::URLLoaderFactory>& factory,
      mojo::Remote<network::mojom::URLLoader>& loader,
      const network::ResourceRequest& request,
      network::TestURLLoaderClient& client) {
    factory->CreateLoaderAndStart(loader.BindNewPipeAndPassReceiver(), 1, 0,
                                  request, client.CreateRemote(),
                                  net::MutableNetworkTrafficAnnotationTag());
  }

  network::ResourceRequest CreateRequest(const GURL& url,
                                         std::optional<url::Origin> initiator) {
    network::ResourceRequest request;
    request.url = url;
    request.request_initiator = std::move(initiator);
    return request;
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<FakeBraveRequestHandler<std::shared_ptr>> request_handler_;
  std::unique_ptr<TestingProfile> profile_;
  network::TestURLLoaderFactory test_factory_;
  std::unique_ptr<BraveProxyingURLLoaderFactory<std::shared_ptr>> proxy_;
};

TEST_F(BraveProxyingURLLoaderFactoryTest, BlocksUnsafeNetworkRedirect) {
  const GURL request_url("https://example.com/source");
  const GURL unsafe_url("data:text/plain,unsafe");
  network::ResourceRequest request = CreateRequest(
      request_url, url::Origin::Create(GURL("https://initiator.example")));
  network::TestURLLoaderFactory::Redirects redirects;
  redirects.emplace_back(CreateRedirectInfo(request, unsafe_url),
                         CreateRedirectHead(unsafe_url));
  test_factory_.AddResponse(request_url, network::mojom::URLResponseHead::New(),
                            "", network::URLLoaderCompletionStatus(net::OK),
                            std::move(redirects));

  auto factory = CreateFactory();
  network::TestURLLoaderClient client;
  mojo::Remote<network::mojom::URLLoader> loader;
  CreateLoaderAndStart(factory, loader, request, client);
  client.RunUntilComplete();

  EXPECT_FALSE(client.has_received_redirect());
  EXPECT_EQ(net::ERR_UNSAFE_REDIRECT, client.completion_status().error_code);
}

TEST_F(BraveProxyingURLLoaderFactoryTest,
       AllowsUnsafeRedirectWhenChecksAreBypassed) {
  const GURL request_url("https://example.com/source");
  const GURL unsafe_url("data:text/plain,safe-from-upstream-proxy");
  network::ResourceRequest request = CreateRequest(
      request_url, url::Origin::Create(GURL("https://initiator.example")));
  auto redirect_head = CreateRedirectHead(unsafe_url);
  redirect_head->bypass_redirect_checks = true;
  network::TestURLLoaderFactory::Redirects redirects;
  redirects.emplace_back(CreateRedirectInfo(request, unsafe_url),
                         std::move(redirect_head));
  test_factory_.AddResponse(request_url, network::mojom::URLResponseHead::New(),
                            "", network::URLLoaderCompletionStatus(net::OK),
                            std::move(redirects));

  auto factory = CreateFactory();
  network::TestURLLoaderClient client;
  mojo::Remote<network::mojom::URLLoader> loader;
  CreateLoaderAndStart(factory, loader, request, client);
  client.RunUntilRedirectReceived();

  EXPECT_TRUE(client.has_received_redirect());
  EXPECT_EQ(unsafe_url, client.redirect_info().new_url);
  EXPECT_TRUE(client.response_head()->bypass_redirect_checks);
}

TEST_F(BraveProxyingURLLoaderFactoryTest,
       TaintsInitiatorOnCrossOriginBraveRedirect) {
  // When the handler rewrites a request to a cross-origin URL (mimicking a
  // Brave static/adblock redirect), the outgoing request's initiator must be
  // tainted to an opaque origin, matching Chromium's WebRequest behavior for
  // step 10 of "4.4. HTTP-redirect fetch"
  // (https://fetch.spec.whatwg.org/#http-redirect-fetch).
  const GURL request_url("https://redirect-source.example/resource");
  const GURL redirected_url("https://redirect-target.example/resource");
  const url::Origin initiator =
      url::Origin::Create(GURL("https://initiator.example"));
  ASSERT_FALSE(initiator.opaque());

  request_handler_->SetRedirect(request_url, redirected_url);

  std::optional<url::Origin> redirected_request_initiator;
  test_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&redirected_request_initiator,
       redirected_url](const network::ResourceRequest& request) {
        if (request.url == redirected_url) {
          redirected_request_initiator = request.request_initiator;
        }
      }));
  test_factory_.AddResponse(redirected_url.spec(), "ok");

  auto factory = CreateFactory();
  network::TestURLLoaderClient client;
  mojo::Remote<network::mojom::URLLoader> loader;
  CreateLoaderAndStart(factory, loader, CreateRequest(request_url, initiator),
                       client);

  client.RunUntilRedirectReceived();
  loader->FollowRedirect({}, std::nullopt);
  client.RunUntilComplete();

  ASSERT_TRUE(redirected_request_initiator.has_value());
  EXPECT_TRUE(redirected_request_initiator->opaque());
  EXPECT_NE(initiator, *redirected_request_initiator);
  EXPECT_EQ(net::OK, client.completion_status().error_code);
}

TEST_F(BraveProxyingURLLoaderFactoryTest,
       PreservesOriginalInitiatorForHandlerAfterBraveRedirect) {
  // Although the outgoing network request's initiator is tainted to an opaque
  // origin after a cross-origin Brave redirect (as verified by
  // TaintsInitiatorOnCrossOriginBraveRedirect), the BraveRequestInfo handed to
  // the request handler for the redirected request must still carry the real,
  // original initiator so Shields/adblock can attribute the request to the true
  // initiator. Using the outgoing request's initiator here would instead expose
  // the opaque tainted origin to the handler.
  const GURL request_url("https://redirect-source.example/resource");
  const GURL redirected_url("https://redirect-target.example/resource");
  ASSERT_TRUE(request_url.is_valid()) << request_url;
  ASSERT_TRUE(redirected_url.is_valid()) << redirected_url;
  const url::Origin initiator =
      url::Origin::Create(GURL("https://initiator.example"));
  ASSERT_FALSE(initiator.opaque()) << initiator;

  request_handler_->SetRedirect(request_url, redirected_url);
  test_factory_.AddResponse(redirected_url.spec(), "ok");

  auto factory = CreateFactory();
  network::TestURLLoaderClient client;
  mojo::Remote<network::mojom::URLLoader> loader;
  CreateLoaderAndStart(factory, loader, CreateRequest(request_url, initiator),
                       client);

  client.RunUntilRedirectReceived();
  loader->FollowRedirect({}, std::nullopt);
  client.RunUntilComplete();

  EXPECT_EQ(net::OK, client.completion_status().error_code)
      << "Request URL: " << request_url
      << ", redirected URL: " << redirected_url;

  bool saw_redirected_request = false;
  std::optional<url::Origin> initiator_for_redirected;
  for (const auto& observation : request_handler_->observations()) {
    if (observation.first == redirected_url) {
      saw_redirected_request = true;
      initiator_for_redirected = observation.second;
    }
  }
  ASSERT_TRUE(saw_redirected_request)
      << "No handler observation for " << redirected_url;
  ASSERT_TRUE(initiator_for_redirected.has_value())
      << "Missing initiator for " << redirected_url;
  EXPECT_FALSE(initiator_for_redirected->opaque())
      << "Redirected URL: " << redirected_url
      << ", initiator: " << *initiator_for_redirected;
  EXPECT_EQ(initiator, *initiator_for_redirected)
      << "Redirected URL: " << redirected_url;
}

}  // namespace
