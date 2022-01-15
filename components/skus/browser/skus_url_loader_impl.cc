// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/browser/skus_url_loader_impl.h"

#include <utility>
#include <vector>

#include "brave/components/skus/browser/rs/cxx/src/lib.rs.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace skus {

SkusUrlLoaderImpl::SkusUrlLoaderImpl(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(url_loader_factory) {}

SkusUrlLoaderImpl::~SkusUrlLoaderImpl() {}

void SkusUrlLoaderImpl::BeginFetch(
    const skus::HttpRequest& req,
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::HttpRoundtripContext>,
                              skus::HttpResponse)> callback,
    rust::cxxbridge1::Box<skus::HttpRoundtripContext> ctx) {
  DCHECK(!simple_url_loader_);
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(static_cast<std::string>(req.url));
  resource_request->method = static_cast<std::string>(req.method);
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  // No cache read, always download from the network.
  resource_request->load_flags =
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;

  for (size_t i = 0; i < req.headers.size(); i++) {
    resource_request->headers.AddHeaderFromString(
        static_cast<std::string>(req.headers[i]));
  }

  simple_url_loader_ = network::SimpleURLLoader::Create(
      std::move(resource_request), GetNetworkTrafficAnnotationTag());

  simple_url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&SkusUrlLoaderImpl::OnFetchComplete,
                     base::Unretained(this), std::move(callback),
                     std::move(ctx)));
}

const net::NetworkTrafficAnnotationTag&
SkusUrlLoaderImpl::GetNetworkTrafficAnnotationTag() {
  static const net::NetworkTrafficAnnotationTag network_traffic_annotation_tag =
      net::DefineNetworkTrafficAnnotation("sku_sdk_execute_request", R"(
      semantics {
        sender: "Brave SKU SDK"
        description:
          "Call the SKU SDK implementation provided by the caller"
        trigger:
          "Any Brave webpage using SKU SDK where window.chrome.braveSkus.*"
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

void SkusUrlLoaderImpl::OnFetchComplete(
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::HttpRoundtripContext>,
                              skus::HttpResponse)> callback,
    rust::cxxbridge1::Box<skus::HttpRoundtripContext> ctx,
    std::unique_ptr<std::string> response_body) {
  uint16_t error_code = simple_url_loader_->NetError();
  uint16_t response_code = net::HTTP_BAD_REQUEST;
  if (simple_url_loader_->ResponseInfo() &&
      simple_url_loader_->ResponseInfo()->headers) {
    response_code =
        simple_url_loader_->ResponseInfo()->headers->response_code();
  }

  bool success = (error_code == net::OK && response_code == net::HTTP_OK);

  std::vector<uint8_t> body_bytes;
  if (response_body) {
    const uint8_t* begin =
        reinterpret_cast<const uint8_t*>(response_body->data());
    const uint8_t* end = begin + response_body->size();
    body_bytes.assign(begin, end);
  }

  skus::HttpResponse resp = {
      success ? skus::SkusResult::Ok : skus::SkusResult::RequestFailed,
      response_code,
      {},
      body_bytes,
  };

  callback(std::move(ctx), resp);
}

}  // namespace skus
