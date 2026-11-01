/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_TEST_SUPPORT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_TEST_SUPPORT_H_

#include <optional>
#include <string>
#include <utility>

#include "base/check_deref.h"
#include "base/json/json_writer.h"
#include "base/test/gmock_expected_support.h"
#include "base/test/protobuf_matchers.h"
#include "base/test/values_test_util.h"
#include "base/types/expected.h"
#include "base/types/is_instantiation.h"
#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/has_to_value.h"
#include "brave/components/brave_account/endpoint_client/is_endpoint.h"
#include "brave/components/brave_account/endpoint_client/is_protobuf_message.h"
#include "brave/components/brave_account/endpoint_client/is_response.h"
#include "brave/components/brave_account/endpoint_client/maybe_strip_with_headers.h"
#include "brave/components/brave_account/endpoint_client/url_replacements.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_account::endpoint_client {

namespace detail {

template <IsJSONResponse Response>
  requires(HasToValue<typename Response::SuccessBody> &&
           HasToValue<typename Response::ErrorBody>)
std::string ToContent(const Response& response) {
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

template <typename Body>
  requires(HasToValue<Body> || IsProtobufMessage<Body>)
testing::Matcher<const Body&> BodyEq(const Body& expected) {
  if constexpr (IsProtobufMessage<Body>) {
    static constexpr auto kKnownFields = [](Body body) {
      body.mutable_unknown_fields()->clear();
      return body;
    };

    return testing::ResultOf("known fields", kKnownFields,
                             base::test::EqualsProto(kKnownFields(expected)));
  } else {
    return testing::ResultOf(
        "ToValue()",
        [](const Body& body) { return base::Value(body.ToValue()); },
        base::test::IsJson(expected.ToValue()));
  }
}

template <typename SuccessBody, typename ErrorBody>
testing::Matcher<const std::optional<base::expected<SuccessBody, ErrorBody>>&>
ResponseBodyEq(
    const std::optional<base::expected<SuccessBody, ErrorBody>>& expected) {
  if (!expected) {
    return testing::Eq(std::nullopt);
  }

  if (expected->has_value()) {
    return testing::Optional(base::test::ValueIs(BodyEq(expected->value())));
  }

  return testing::Optional(base::test::ErrorIs(BodyEq(expected->error())));
}

}  // namespace detail

// Registers a mock response for Endpoint in network::TestURLLoaderFactory.
// Supports simulating transport errors, HTTP status codes, and
// success/error bodies using Endpoint::Response.
// When response.body is std::nullopt, emits malformed content so that
// deserialization yields std::nullopt, matching the observable result
// that cannot be expressed directly via TestURLLoaderFactory,
// since it cannot model the absence of a response payload.
// |url_replacements| must match the request's url_replacements, so the mock is
// registered against the same URL that Client::Send() will actually request.
template <IsEndpoint Endpoint>
void MockResponseFor(network::TestURLLoaderFactory& test_url_loader_factory,
                     const typename Endpoint::Response& response,
                     const UrlReplacements& url_replacements = {}) {
  auto head = response.status_code
                  .transform([](auto status_code) {
                    return network::CreateURLResponseHead(
                        static_cast<net::HttpStatusCode>(status_code));
                  })
                  .value_or(network::mojom::URLResponseHead::New());

  test_url_loader_factory.AddResponse(
      url_replacements.Apply(Endpoint::URL()), std::move(head),
      detail::ToContent(response),
      network::URLLoaderCompletionStatus(response.net_error));
}

// Returns true if a network::ResourceRequest matches Endpoint,
// based on Endpoint::URL() and Endpoint::Request::Method().
// Intended for use with network::TestURLLoaderFactory interceptors in tests
// where multiple endpoints share the same URL but differ only by method.
// |url_replacements| must match the request's url_replacements, so the request
// is compared against the same URL that Client::Send() will actually request.
template <IsEndpoint Endpoint>
bool MatchesEndpoint(const network::ResourceRequest& request,
                     const UrlReplacements& url_replacements = {}) {
  return request.url == url_replacements.Apply(Endpoint::URL()) &&
         request.method == Endpoint::Request::Method();
}

template <typename Response>
  requires detail::IsResponse<detail::MaybeStripWithHeaders<Response>>
testing::Matcher<const Response&> ResponseEq(const Response& expected) {
  auto response_matcher = testing::AllOf(
      testing::Field("net_error", &Response::net_error, expected.net_error),
      testing::Field("status_code", &Response::status_code,
                     expected.status_code),
      testing::Field("body", &Response::body,
                     detail::ResponseBodyEq(expected.body)));

  if constexpr (base::is_instantiation<Response, WithHeaders>) {
    static constexpr auto kRawHeaders = [](const auto& headers) {
      return headers ? std::optional(headers->raw_headers()) : std::nullopt;
    };

    return testing::AllOf(
        response_matcher,
        testing::Field("headers", &Response::headers,
                       testing::ResultOf("raw headers", kRawHeaders,
                                         kRawHeaders(expected.headers))));
  }

  return response_matcher;
}

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_TEST_SUPPORT_H_
