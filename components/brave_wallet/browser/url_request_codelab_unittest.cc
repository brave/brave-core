// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "base/memory/scoped_refptr.h"
#include "base/task/single_thread_task_runner.h"
#include "base/test/bind.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "net/proxy_resolution/proxy_config_service_fixed.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_context_getter.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/transitional_url_loader_factory_owner.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

// ONLY as a demonstration. Tests should not do internet calls.
class URLRequestContextGetter : public net::URLRequestContextGetter {
 public:
  explicit URLRequestContextGetter(
      scoped_refptr<base::SingleThreadTaskRunner> network_task_runner)
      : network_task_runner_(network_task_runner) {}

  net::URLRequestContext* GetURLRequestContext() override {
    if (!url_request_context_) {
      net::URLRequestContextBuilder builder;

      builder.set_user_agent("lol");
      builder.DisableHttpCache();
      builder.set_proxy_config_service(
          std::make_unique<net::ProxyConfigServiceFixed>(
              net::ProxyConfigWithAnnotation::CreateDirect()));
      url_request_context_ = builder.Build();
    }
    return url_request_context_.get();
  }

  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override {
    return network_task_runner_;
  }

 private:
  ~URLRequestContextGetter() override = default;

  scoped_refptr<base::SingleThreadTaskRunner> network_task_runner_;

  std::unique_ptr<net::URLRequestContext> url_request_context_;
};

}  // namespace

class UrlRequestCodelabTest : public testing::Test {
 public:
  UrlRequestCodelabTest()
      : url_request_context_getter_(new URLRequestContextGetter(
            task_environment_.GetMainThreadTaskRunner())),
        url_loader_factory_owner_(url_request_context_getter_) {}

  network::SharedURLLoaderFactory* GetURLLoaderFactory() {
    return url_loader_factory_owner_.GetURLLoaderFactory().get();
  }

 private:
  base::test::TaskEnvironment task_environment_;
  scoped_refptr<net::URLRequestContextGetter> url_request_context_getter_;
  network::TransitionalURLLoaderFactoryOwner url_loader_factory_owner_;
};

TEST_F(UrlRequestCodelabTest, Basics) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL("https://www.google.com");
  resource_request->method = "GET";
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->headers.SetHeader("Accept", "text/html");
  auto loader = network::SimpleURLLoader::Create(std::move(resource_request),
                                                 MISSING_TRAFFIC_ANNOTATION);

  bool received_response = false;
  loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      GetURLLoaderFactory(),
      base::BindLambdaForTesting(
          [&received_response](std::optional<std::string> response_body) {
            LOG(INFO) << "Response: " << response_body.value_or("null");
            received_response = true;
          }));

  ASSERT_TRUE(base::test::RunUntil(
      [&received_response]() { return received_response; }));
}
