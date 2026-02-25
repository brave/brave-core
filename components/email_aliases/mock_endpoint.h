/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EMAIL_ALIASES_MOCK_ENDPOINT_H_
#define BRAVE_COMPONENTS_EMAIL_ALIASES_MOCK_ENDPOINT_H_

#include <functional>
#include <optional>
#include <utility>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_account/endpoint_client/is_endpoint.h"
#include "brave/components/brave_account/endpoint_client/request.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "brave/components/brave_account/endpoint_client/test_support.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/url_loader_completion_status.h"
#include "services/network/test/test_url_loader_factory.h"
#include "services/network/test/test_utils.h"

namespace email_aliases::test {

namespace detail {

using brave_account::endpoint_client::IsEndpoint;

template <typename T>
struct EndpointRequestBody;

template <typename T, brave_account::endpoint_client::detail::Method M>
struct EndpointRequestBody<
    brave_account::endpoint_client::detail::Request<T, M>> {
  using type = T;
};

template <brave_account::endpoint_client::IsEndpoint Endpoint>
using EndpointRequestBodyT =
    typename EndpointRequestBody<typename Endpoint::Request>::type;

template <typename Endpoint, typename F, typename Out>
concept IsProducer = requires(F f, EndpointRequestBodyT<Endpoint> body) {
  { f(std::move(body)) } -> std::convertible_to<Out>;
} || requires(F f) {
  { f() } -> std::convertible_to<Out>;
} || std::convertible_to<F, Out>;

template <typename F, typename Endpoint>
concept IsResponseProducer =
    IsEndpoint<Endpoint> &&
    detail::IsProducer<Endpoint, F, typename Endpoint::Response>;

template <typename F, typename Endpoint>
concept IsSuccessBodyProducer =
    IsEndpoint<Endpoint> &&
    detail::IsProducer<Endpoint, F, typename Endpoint::Response::SuccessBody>;

template <typename F, typename Endpoint>
concept IsErrorBodyProducer =
    IsEndpoint<Endpoint> &&
    detail::IsProducer<Endpoint, F, typename Endpoint::Response::ErrorBody>;

template <typename F, typename Endpoint>
concept IsAcceptableProducer =
    IsResponseProducer<F, Endpoint> || IsSuccessBodyProducer<F, Endpoint> ||
    IsErrorBodyProducer<F, Endpoint>;

template <IsEndpoint Endpoint>
EndpointRequestBodyT<Endpoint> GetEndpointRequestBody(
    const network::ResourceRequest& request) {
  const std::string_view upload = [&]() -> std::string_view {
    if (!request.request_body || request.request_body->elements()->empty()) {
      return {};
    }
    return request.request_body->elements()
        ->at(0)
        .As<network::DataElementBytes>()
        .AsStringPiece();
  }();

  auto endpoint_request =
      Endpoint::Request::FromValue(
          base::JSONReader::Read(upload, base::JSON_PARSE_RFC)
              .value_or(base::Value(base::DictValue())))
          .value();

  return endpoint_request;
}

template <IsEndpoint Endpoint>
typename Endpoint::Response MakeEndpointResponse(
    const network::ResourceRequest& request,
    typename Endpoint::Response response) {
  return response;
}

template <IsEndpoint Endpoint, IsResponseProducer<Endpoint> F>
typename Endpoint::Response MakeEndpointResponse(
    const network::ResourceRequest& request,
    F&& producer) {
  if constexpr (std::is_invocable_v<F>) {
    return std::invoke(std::forward<F>(producer));
  } else {
    return std::invoke(std::forward<F>(producer),
                       GetEndpointRequestBody<Endpoint>(request));
  }
}

template <IsEndpoint Endpoint, IsSuccessBodyProducer<Endpoint> F>
typename Endpoint::Response MakeEndpointResponse(
    const network::ResourceRequest& request,
    F&& producer) {
  if constexpr (std::is_invocable_v<F>) {
    auto success_body = std::invoke(std::forward<F>(producer));
    return {.net_error = net::OK,
            .status_code = net::HTTP_OK,
            .body = std::move(success_body)};
  } else {
    auto success_body = std::invoke(std::forward<F>(producer),
                                    GetEndpointRequestBody<Endpoint>(request));
    return {.net_error = net::OK,
            .status_code = net::HTTP_OK,
            .body = std::move(success_body)};
  }
}

template <IsEndpoint Endpoint, IsErrorBodyProducer<Endpoint> F>
typename Endpoint::Response MakeEndpointResponse(
    const network::ResourceRequest& request,
    F&& producer) {
  if constexpr (std::is_invocable_v<F>) {
    auto error_body = std::invoke(std::forward<F>(producer));
    return {.net_error = net::OK,
            .status_code = net::HTTP_BAD_REQUEST,
            .body = base::unexpected(std::move(error_body))};
  } else {
    auto error_body = std::invoke(std::forward<F>(producer),
                                  GetEndpointRequestBody<Endpoint>(request));
    return {.net_error = net::OK,
            .status_code = net::HTTP_BAD_REQUEST,
            .body = base::unexpected(std::move(error_body))};
  }
}

}  // namespace detail

// Returns endpoint request's upload body type.
template <detail::IsEndpoint Endpoint>
using EndpointRequestBody = detail::EndpointRequestBodyT<Endpoint>;

// Extracts and parses the request body into the endpoint's request type
using detail::GetEndpointRequestBody;

// The MockResponseFor function and its helpers provide a testing utility for
// mocking network requests to predefined API endpoints.
//
// |response_producer| generate responses from various producer types:
// Endpoint::Response object - returned as is.
// Response producers (function which returns Encpoint::Response)
// Success body producers (function which returns Response::SuccessBody)
// Error body producers (function which returns Response::ErrorBody)
//
// * Producers can be stateless ([]() -> T) or use request body ([](const
// Endpoint::RequestBody&) -> T)

// Using a lambda that takes the request body

// test_url_loader_factory.SetInterceptor(base::BindLambdaForTesting(
//         [&](const network::ResourceRequest& request) {
//   MockResponseFor<CreateAccountEndpoint>(test_url_loader_factory, request,
//                                       [](const CreateRequestBody& body) {
//                                         return AccountCreated{body.user_id};
//                                       });
//   });

// Using a stateless lambda:
// test_url_loader_factory.SetInterceptor(
//     base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
//       MockResponseFor<CreateAccountEndpoint>(test_url_loader_factory,
//       request,
//       []() {
//         return AccountCreated{123};
//       });
//     }));

// Using a pre-built response object:
// test_url_loader_factory.SetInterceptor(
//     base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
//       MockResponseFor<CreateAccountEndpoint>(
//           test_url_loader_factory, request,
//           Endpoint::Response{.net_error = net::OK,
//                              .status_code = net::HTTP_CREATED,
//                              .body = AccountCreated{"user123"}})
//     }));

// Mocking Error Responses:
// test_url_loader_factory.SetInterceptor(
//     base::BindLambdaForTesting([&](const network::ResourceRequest& request) {
//       MockResponseFor<CreateAccountEndpoint>(
//           test_url_loader_factory, request, []() {
//             return ApiError{"Invalid email format"};
//           })
//     }));

template <detail::IsEndpoint Endpoint>
void MockResponseFor(network::TestURLLoaderFactory& test_url_loader_factory,
                     std::string_view raw_body,
                     std::optional<int> status_code = net::HTTP_OK,
                     int net_error = net::OK) {
  auto head = status_code
                  .transform([](auto status_code) {
                    return network::CreateURLResponseHead(
                        static_cast<net::HttpStatusCode>(status_code));
                  })
                  .value_or(nullptr);

  test_url_loader_factory.AddResponse(
      Endpoint::URL(), std::move(head), raw_body,
      network::URLLoaderCompletionStatus(net_error));
}

template <detail::IsEndpoint Endpoint,
          detail::IsAcceptableProducer<Endpoint> ResponseProducer =
              typename Endpoint::Response>
void MockResponseFor(network::TestURLLoaderFactory& test_url_loader_factory,
                     const network::ResourceRequest& request,
                     ResponseProducer&& response_producer) {
  if (!brave_account::endpoint_client::MatchesEndpoint<Endpoint>(request)) {
    return;
  }

  auto response = detail::MakeEndpointResponse<Endpoint>(
      request, std::forward<ResponseProducer>(response_producer));

  const auto body = base::WriteJson(response.body
                                        .transform([](const auto& body) {
                                          return body.has_value()
                                                     ? body.value().ToValue()
                                                     : body.error().ToValue();
                                        })
                                        .value_or(base::DictValue()))
                        .value();

  MockResponseFor<Endpoint>(test_url_loader_factory, body,
                            std::move(response.status_code),
                            response.net_error);
}

template <detail::IsEndpoint Endpoint,
          detail::IsAcceptableProducer<Endpoint> ResponseProducer =
              typename Endpoint::Response>
void MockResponseFor(network::TestURLLoaderFactory& test_url_loader_factory,
                     ResponseProducer&& response_producer) {
  network::ResourceRequest request;
  request.url = Endpoint::URL();
  request.method = Endpoint::Request::Method();
  MockResponseFor<Endpoint>(test_url_loader_factory, request,
                            std::forward<ResponseProducer>(response_producer));
}

}  // namespace email_aliases::test

#endif  // BRAVE_COMPONENTS_EMAIL_ALIASES_MOCK_ENDPOINT_H_
