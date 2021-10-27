// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_FETCHER_H_
#define BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_FETCHER_H_

#include <memory>
#include <string>

#include "brave/components/skus/browser/brave-rewards-cxx/src/cxx.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace brave_rewards {

struct HttpRequest;
struct HttpResponse;
struct HttpRoundtripContext;

class SkusSdkFetcher {
 public:
  explicit SkusSdkFetcher(scoped_refptr<network::SharedURLLoaderFactory>);
  ~SkusSdkFetcher();

  void BeginFetch(
      const brave_rewards::HttpRequest& req,
      rust::cxxbridge1::Fn<
          void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>,
               brave_rewards::HttpResponse)> callback,
      rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> ctx);

 private:
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> sku_sdk_loader_;

  const net::NetworkTrafficAnnotationTag& GetNetworkTrafficAnnotationTag();

  void OnFetchComplete(
      rust::cxxbridge1::Fn<
          void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>,
               brave_rewards::HttpResponse)> callback,
      rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> ctx,
      std::unique_ptr<std::string> response_body);
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_SKUS_BROWSER_SKUS_SDK_FETCHER_H_
