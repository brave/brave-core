/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_CLIENT_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_CLIENT_H_

#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/types/is_instantiation.h"
#include "brave/components/endpoint_client/is_endpoint.h"
#include "brave/components/endpoint_client/is_request.h"
#include "brave/components/endpoint_client/maybe_strip_with_headers.h"
#include "brave/components/endpoint_client/request_handle.h"
#include "brave/components/endpoint_client/request_options.h"
#include "brave/components/endpoint_client/with_headers.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_loader_factory.mojom-forward.h"
#include "url/gurl.h"

// See //brave/components/endpoint_client/README.md
// for design, motivation, usage, and examples.

namespace endpoint_client {

enum class RequestCancelability { kNonCancelable, kCancelable };

namespace internal {

using OnResponseCallback =
    base::OnceCallback<void(int net_error,
                            scoped_refptr<net::HttpResponseHeaders> headers,
                            std::optional<std::string> response_body)>;

// Creates and starts a `SimpleURLLoader` for the request. For
// `RequestCancelability::kCancelable`, returns the loader, and destroying it
// cancels the request. Otherwise, the loader is owned by its own completion
// callback, which destroys it once the response has been handled, and this
// returns `nullptr`.
std::unique_ptr<network::SimpleURLLoader> StartSimpleURLLoader(
    network::mojom::URLLoaderFactory& url_loader_factory,
    std::string_view method,
    const GURL& url,
    net::HttpRequestHeaders headers,
    std::optional<std::string> upload_data,
    std::string_view content_type,
    RequestOptions options,
    RequestCancelability cancelability,
    OnResponseCallback on_response_callback);

}  // namespace internal

// Client<> performs a full network roundtrip for a given endpoint using
// network::SimpleURLLoader. It serializes the endpoint's Request type,
// performs the network call, and invokes a callback with the response
// deserialized as the endpoint's Response type. Requests and responses may
// optionally be wrapped in WithHeaders<> to include HTTP headers. Requests
// can also be made cancelable, in which case Send<>() returns a RequestHandle
// that can cancel the request.
template <IsEndpoint Endpoint>
class Client {
 public:
  // [[nodiscard]] enforces that callers retain the returned handle in
  // the cancelable case. It has no effect in the non-cancelable case,
  // where the return type is void.
  template <RequestCancelability C = RequestCancelability::kNonCancelable,
            typename Request,
            typename Response>
    requires(std::same_as<internal::MaybeStripWithHeaders<Request>,
                          typename Endpoint::Request> &&
             std::same_as<internal::MaybeStripWithHeaders<Response>,
                          typename Endpoint::Response>)
  [[nodiscard]] static auto Send(
      network::mojom::URLLoaderFactory& url_loader_factory,
      Request request,
      base::OnceCallback<void(Response)> callback) {
    auto headers = [&] {
      if constexpr (base::is_instantiation<Request, WithHeaders>) {
        return std::move(request.headers);
      } else {
        return net::HttpRequestHeaders();
      }
    }();

    constexpr bool kIsBodylessRequest =
        internal::IsBodylessRequest<typename Endpoint::Request>;

    auto upload_data = [&]() -> std::optional<std::string> {
      if constexpr (kIsBodylessRequest) {
        return std::nullopt;
      } else {
        return request.Serialize();
      }
    }();

    constexpr auto kContentType = [] {
      if constexpr (kIsBodylessRequest) {
        return std::string_view();
      } else {
        return Request::ContentType();
      }
    }();

    auto simple_url_loader = internal::StartSimpleURLLoader(
        url_loader_factory, Request::Method(), Endpoint::URL(),
        std::move(headers), std::move(upload_data), kContentType,
        std::move(static_cast<internal::RequestOptions&>(request)), C,
        base::BindOnce(MakeResponse<Response>).Then(std::move(callback)));

    if constexpr (C == RequestCancelability::kCancelable) {
      return RequestHandle(std::move(simple_url_loader),
                           base::SequencedTaskRunner::GetCurrentDefault());
    }
  }

 private:
  template <typename Response>
  static Response MakeResponse(int net_error,
                               scoped_refptr<net::HttpResponseHeaders> headers,
                               std::optional<std::string> response_body) {
    Response response(Response::Deserialize(net_error, headers.get(),
                                            std::move(response_body)));
    if constexpr (base::is_instantiation<Response, WithHeaders>) {
      if (response.status_code) {
        response.headers = std::move(headers);
      }
    }

    return response;
  }
};

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_CLIENT_H_
