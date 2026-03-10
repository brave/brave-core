/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_TEST_SUPPORT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_TEST_SUPPORT_H_

#include <utility>

#include "base/check_deref.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_account/endpoint_client/is_endpoint.h"
#include "brave/components/brave_account/endpoint_client/is_response.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"

namespace brave_account::endpoint_client {

namespace detail {

std::string ToContent(const IsJSONResponse auto& response) {
  return response.body
      .transform([](const auto& body) {
        return CHECK_DEREF(base::WriteJson(body.has_value()
                                               ? body.value().ToValue()
                                               : body.error().ToValue()));
      })
      .value_or(R"({"invalid": json})");  // malformed JSON
}

std::string ToContent(const IsProtobufResponse auto& response) {
  return response.body
      .transform([](const auto& body) {
        return body.has_value() ? body.value().SerializeAsString()
                                : body.error().SerializeAsString();
      })
      .value_or("\x0A");  // malformed Protobuf
}

}  // namespace detail

// Registers a mock response for Endpoint in network::TestURLLoaderFactory.
// Supports simulating transport errors, HTTP status codes, and
// success/error bodies using Endpoint::Response.
// When response.body is std::nullopt, emits malformed content so that
// deserialization yields std::nullopt, matching the observable result
// that cannot be expressed directly via TestURLLoaderFactory,
// since it cannot model the absence of a response payload.
template <IsEndpoint Endpoint>
void MockResponseFor(network::TestURLLoaderFactory& test_url_loader_factory,
                     const typename Endpoint::Response& response) {
  auto head = response.status_code
                  .transform([](auto status_code) {
                    return network::CreateURLResponseHead(
                        static_cast<net::HttpStatusCode>(status_code));
                  })
                  .value_or(network::mojom::URLResponseHead::New());

  test_url_loader_factory.AddResponse(
      Endpoint::URL(), std::move(head), detail::ToContent(response),
      network::URLLoaderCompletionStatus(response.net_error));
}

// Returns true if a network::ResourceRequest matches Endpoint,
// based on Endpoint::URL() and Endpoint::Request::Method().
// Intended for use with network::TestURLLoaderFactory interceptors in tests
// where multiple endpoints share the same URL but differ only by method.
template <IsEndpoint Endpoint>
bool MatchesEndpoint(const network::ResourceRequest& request) {
  return request.url == Endpoint::URL() &&
         request.method == Endpoint::Request::Method();
}

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_TEST_SUPPORT_H_
