/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_proxying_url_loader_factory.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/test/bind.h"
#include "base/test/task_environment.h"
#include "brave/browser/net/brave_request_handler.h"
#include "brave/browser/net/url_context.h"
#include "chrome/test/base/testing_profile.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_util.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/cpp/url_loader_factory_builder.h"
#include "services/network/test/test_url_loader_client.h"
#include "services/network/test/test_url_loader_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
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
  BraveProxyingURLLoaderFactoryTest()
      : request_handler_(), profile_(TestingProfile::Builder().Build()) {}

  mojo::Remote<network::mojom::URLLoaderFactory> CreateFactory() {
    network::URLLoaderFactoryBuilder builder;
    proxy_ = std::make_unique<BraveProxyingURLLoaderFactory<std::shared_ptr>>(
        request_handler_, profile_.get(), content::GlobalRenderFrameHostToken(),
        builder, base::MakeRefCounted<RequestIDGenerator>(), base::DoNothing(),
        nullptr);

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

  network::ResourceRequest CreateRequest(
      const GURL& url,
      const std::optional<url::Origin>& initiator) {
    network::ResourceRequest request;
    request.url = url;
    request.request_initiator = initiator;
    return request;
  }

 protected:
  content::BrowserTaskEnvironment task_environment_;
  BraveRequestHandler<std::shared_ptr> request_handler_;
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
  task_environment_.RunUntilIdle();

  EXPECT_FALSE(client.has_received_redirect());
  ASSERT_TRUE(client.has_received_completion());
  EXPECT_EQ(net::ERR_UNSAFE_REDIRECT, client.completion_status().error_code);
}

TEST_F(BraveProxyingURLLoaderFactoryTest,
       PreservesOriginalInitiatorAcrossSyntheticRedirect) {
  const GURL request_url("https://example.com/source");
  const GURL first_redirect_url("https://other.example/first");
  const GURL second_redirect_url("https://third.example/second");
  const url::Origin initiator =
      url::Origin::Create(GURL("https://initiator.example"));

  request_handler_.SetBeforeURLRequestCallbackForTesting(
      base::BindLambdaForTesting(
          [request_url, first_redirect_url, second_redirect_url](
              const brave::ResponseCallback&,
              std::shared_ptr<brave::BraveRequestInfo> ctx) {
            if (ctx->request_url() == request_url) {
              ctx->set_new_url_spec(first_redirect_url.spec());
            } else if (ctx->request_url() == first_redirect_url) {
              ctx->set_new_url_spec(second_redirect_url.spec());
            }
            return net::OK;
          }));

  std::optional<url::Origin> second_request_initiator;
  test_factory_.SetInterceptor(base::BindLambdaForTesting(
      [&second_request_initiator,
       second_redirect_url](const network::ResourceRequest& request) {
        if (request.url == second_redirect_url) {
          second_request_initiator = request.request_initiator;
        }
      }));
  test_factory_.AddResponse(second_redirect_url.spec(), "ok");

  auto factory = CreateFactory();
  network::TestURLLoaderClient client;
  mojo::Remote<network::mojom::URLLoader> loader;
  CreateLoaderAndStart(factory, loader, CreateRequest(request_url, initiator),
                       client);

  client.RunUntilRedirectReceived();
  loader->FollowRedirect({}, std::nullopt);
  client.ClearHasReceivedRedirect();
  client.RunUntilRedirectReceived();
  loader->FollowRedirect({}, std::nullopt);
  client.RunUntilComplete();

  ASSERT_TRUE(second_request_initiator.has_value());
  EXPECT_EQ(initiator, *second_request_initiator);
  EXPECT_EQ(net::OK, client.completion_status().error_code);
}

}  // namespace
