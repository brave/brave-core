// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/skus/browser/skus_url_loader_impl.h"

#include <utility>
#include <vector>

#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/skus/browser/rs/cxx/src/lib.rs.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace skus {
namespace {
const net::NetworkTrafficAnnotationTag& GetNetworkTrafficAnnotationTag() {
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

}  // namespace

SkusUrlLoaderImpl::SkusUrlLoaderImpl(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(new api_request_helper::APIRequestHelper(
          GetNetworkTrafficAnnotationTag(),
          url_loader_factory)) {}

SkusUrlLoaderImpl::~SkusUrlLoaderImpl() = default;

void SkusUrlLoaderImpl::BeginFetch(
    const skus::HttpRequest& req,
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::HttpRoundtripContext>,
                              skus::HttpResponse)> callback,
    rust::cxxbridge1::Box<skus::HttpRoundtripContext> ctx) {
  base::flat_map<std::string, std::string> headers;
  for (const auto& header : req.headers) {
    std::vector<std::string> lines =
        base::SplitString(static_cast<std::string>(header), ": ",
                          base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    if (lines.size() != 2)
      continue;

    headers[lines.front()] = lines.back();
  }
  Request(static_cast<std::string>(req.method),
          GURL(static_cast<std::string>(req.url)), std::string(),
          "application/json", true,
          base::BindOnce(&SkusUrlLoaderImpl::OnFetchComplete,
                         base::Unretained(this), std::move(callback),
                         std::move(ctx)),
          headers, -1u);
}

void SkusUrlLoaderImpl::Request(
    const std::string& method,
    const GURL& url,
    const std::string& payload,
    const std::string& payload_content_type,
    bool auto_retry_on_network_change,
    api_request_helper::APIRequestHelper::ResultCallback callback,
    const base::flat_map<std::string, std::string>& headers,
    size_t max_body_size /* = -1u */) {
  api_request_helper_->Request(method, url, payload, payload_content_type,
                               auto_retry_on_network_change,
                               std::move(callback), headers, max_body_size);
}

void SkusUrlLoaderImpl::OnFetchComplete(
    rust::cxxbridge1::Fn<void(rust::cxxbridge1::Box<skus::HttpRoundtripContext>,
                              skus::HttpResponse)> callback,
    rust::cxxbridge1::Box<skus::HttpRoundtripContext> ctx,
    api_request_helper::APIRequestResult api_request_result) {
  uint16_t response_code = api_request_result.response_code();
  bool success = api_request_result.Is2XXResponseCode();
  std::vector<uint8_t> body_bytes;
  if (!api_request_result.body().empty()) {
    body_bytes.assign(api_request_result.body().begin(),
                      api_request_result.body().end());
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
