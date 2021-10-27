// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/browser/skus_sdk_fetcher.h"

#include <utility>
#include <vector>

#include "brave/components/skus/browser/brave-rewards-cxx/src/wrapper.h"
#include "net/base/load_flags.h"
#include "url/gurl.h"

namespace {

// TODO(bsclifton): fix me. I set a completely arbitrary size!
const int kMaxResponseSize = 1000000;  // 1Mb

}  // namespace

namespace brave_rewards {

SkusSdkFetcher::SkusSdkFetcher(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory) {}

SkusSdkFetcher::~SkusSdkFetcher() {}

void SkusSdkFetcher::BeginFetch(
    const brave_rewards::HttpRequest& req,
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>,
             brave_rewards::HttpResponse)> callback,
    rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> ctx) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(static_cast<std::string>(req.url));
  resource_request->method = static_cast<std::string>(req.method).c_str();
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  // No cache read, always download from the network.
  resource_request->load_flags =
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;

  for (size_t i = 0; i < req.headers.size(); i++) {
    resource_request->headers.AddHeaderFromString(
        static_cast<std::string>(req.headers[i]));
  }

  sku_sdk_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), GetNetworkTrafficAnnotationTag());

  sku_sdk_loader_->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&SkusSdkFetcher::OnFetchComplete, base::Unretained(this),
                     std::move(callback), std::move(ctx)),
      kMaxResponseSize);
}

const net::NetworkTrafficAnnotationTag&
SkusSdkFetcher::GetNetworkTrafficAnnotationTag() {
  static const net::NetworkTrafficAnnotationTag network_traffic_annotation_tag =
      net::DefineNetworkTrafficAnnotation("sku_sdk_execute_request", R"(
      semantics {
        sender: "Brave SKU SDK"
        description:
          "Call the SKU SDK implementation provided by the caller"
        trigger:
          "Any Brave webpage using SKU SDK where window.brave.sku.*"
          "methods are called; ex: fetch_order / fetch_order_credentials"
        data: "JSON data comprising an order."
        destination: OTHER
        destination_other: "Brave developers"
      }
      policy {
        cookies_allowed: NO
      })");
  return network_traffic_annotation_tag;
}

void SkusSdkFetcher::OnFetchComplete(
    rust::cxxbridge1::Fn<
        void(rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext>,
             brave_rewards::HttpResponse)> callback,
    rust::cxxbridge1::Box<brave_rewards::HttpRoundtripContext> ctx,
    std::unique_ptr<std::string> response_body) {
  if (!response_body) {
    std::vector<uint8_t> body_bytes;
    brave_rewards::HttpResponse resp = {
        brave_rewards::RewardsResult::RequestFailed,
        500,
        {},
        body_bytes,
    };
    callback(std::move(ctx), resp);
    return;
  }

  std::vector<uint8_t> body_bytes(response_body->begin(), response_body->end());

  brave_rewards::HttpResponse resp = {
      brave_rewards::RewardsResult::Ok,
      200,
      {},
      body_bytes,
  };

  callback(std::move(ctx), resp);
}

}  // namespace brave_rewards
