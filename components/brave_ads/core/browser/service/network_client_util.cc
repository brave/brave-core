/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/network_client_util.h"

#include <cstddef>

#include "base/check.h"
#include "base/notreached.h"
#include "brave/components/brave_ads/core/browser/service/oblivious_http_constants.h"
#include "brave/components/brave_ads/core/browser/service/oblivious_http_feature.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"
#include "url/gurl.h"

namespace brave_ads {

GURL ObliviousHttpKeyConfigUrl(bool use_staging) {
  return GURL(use_staging ? kStagingObliviousHttpKeyConfigUrl
                          : kProductionObliviousHttpKeyConfigUrl);
}

GURL ObliviousHttpRelayUrl(bool use_staging) {
  return GURL(use_staging ? kStagingObliviousHttpRelayUrl
                          : kProductionObliviousHttpRelayUrl);
}

std::string ToString(mojom::UrlRequestMethodType value) {
  CHECK(mojom::IsKnownEnumValue(value));

  switch (value) {
    case mojom::UrlRequestMethodType::kGet: {
      return net::HttpRequestHeaders::kGetMethod;
    }

    case mojom::UrlRequestMethodType::kPost: {
      return net::HttpRequestHeaders::kPostMethod;
    }

    case mojom::UrlRequestMethodType::kPut: {
      return net::HttpRequestHeaders::kPutMethod;
    }
  }

  NOTREACHED() << "Unexpected value for mojom::UrlRequestMethodType: " << value;
}

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("brave_ads", R"(
      semantics {
        sender: "Brave Ads"
        description:
          "This service is used to communicate with Brave servers "
          "to send and retrieve information for Ads."
        trigger:
          "Triggered by user viewing ads or at various intervals."
        data:
          "Ads catalog and Confirmations."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature by visiting brave://rewards."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

network::mojom::ObliviousHttpRequestPtr BuildObliviousHttpRequest(
    const GURL& relay_url,
    const std::string& key_config,
    const mojom::UrlRequestInfoPtr& mojom_url_request) {
  auto mojom_http_request = network::mojom::ObliviousHttpRequest::New();
  mojom_http_request->relay_url = relay_url;
  mojom_http_request->traffic_annotation =
      net::MutableNetworkTrafficAnnotationTag(GetNetworkTrafficAnnotationTag());
  mojom_http_request->timeout_duration = kOhttpTimeoutDuration.Get();
  mojom_http_request->key_config = key_config;
  mojom_http_request->resource_url = mojom_url_request->url;
  mojom_http_request->method = ToString(mojom_url_request->method);
  mojom_http_request->request_body =
      network::mojom::ObliviousHttpRequestBody::New(
          mojom_url_request->content, mojom_url_request->content_type);
  return mojom_http_request;
}

base::flat_map<std::string, std::string> ExtractHttpResponseHeaders(
    const scoped_refptr<net::HttpResponseHeaders>& http_response_headers) {
  CHECK(http_response_headers);

  size_t iter = 0;
  std::string key;
  std::string value;

  base::flat_map<std::string, std::string> headers;
  while (http_response_headers->EnumerateHeaderLines(&iter, &key, &value)) {
    key = base::ToLowerASCII(key);
    headers[key] = value;
  }

  return headers;
}

}  // namespace brave_ads
