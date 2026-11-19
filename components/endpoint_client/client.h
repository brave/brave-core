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

#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/types/is_instantiation.h"
#include "brave/components/endpoint_client/deserialize_body.h"
#include "brave/components/endpoint_client/is_endpoint.h"
#include "brave/components/endpoint_client/is_request.h"
#include "brave/components/endpoint_client/is_response.h"
#include "brave/components/endpoint_client/is_stream.h"
#include "brave/components/endpoint_client/maybe_strip_with_headers.h"
#include "brave/components/endpoint_client/request_handle.h"
#include "brave/components/endpoint_client/request_options.h"
#include "brave/components/endpoint_client/stream_consumer.h"
#include "brave/components/endpoint_client/with_headers.h"
#include "net/base/net_errors.h"
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
// returns `nullptr`. For `download_headers_only`, the response body is
// discarded as it's received, and `max_body_size` doesn't apply.
std::unique_ptr<network::SimpleURLLoader> StartSimpleURLLoader(
    network::mojom::URLLoaderFactory& url_loader_factory,
    std::string_view method,
    const GURL& url,
    net::HttpRequestHeaders headers,
    std::optional<std::string> upload_data,
    std::string_view content_type,
    RequestOptions options,
    RequestCancelability cancelability,
    bool download_headers_only,
    OnResponseCallback on_response_callback);

// Creates and starts a `SimpleURLLoader` that streams the response to
// `callbacks`, and returns its consumer, which owns it. Destroying the
// consumer cancels the request. `max_body_size` only caps the buffered error
// body, and `retry_options` doesn't apply.
std::unique_ptr<StreamConsumer> StartStreamingSimpleURLLoader(
    network::mojom::URLLoaderFactory& url_loader_factory,
    std::string_view method,
    const GURL& url,
    net::HttpRequestHeaders headers,
    std::optional<std::string> upload_data,
    std::string_view content_type,
    RequestOptions options,
    bool buffer_error_body,
    RawStreamCallbacks callbacks);

struct SerializedRequest {
  net::HttpRequestHeaders headers;
  std::optional<std::string> upload_data;
  std::string_view content_type;
};

}  // namespace internal

// Client<> performs a full network roundtrip for a given endpoint using
// network::SimpleURLLoader. It serializes the endpoint's Request type,
// performs the network call, and invokes a callback with the response
// deserialized as the endpoint's Response type. Requests and responses may
// optionally be wrapped in WithHeaders<> to include HTTP headers. Requests
// can also be made cancelable, in which case Send<>() returns a RequestHandle
// that can cancel the request. For endpoints whose Response is a Stream<>,
// Send<>() streams the response body to callbacks instead.
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
    auto serialized_request = SerializeRequest(request);
    auto simple_url_loader = internal::StartSimpleURLLoader(
        url_loader_factory, Request::Method(), Endpoint::URL(),
        std::move(serialized_request.headers),
        std::move(serialized_request.upload_data),
        serialized_request.content_type,
        std::move(static_cast<internal::RequestOptions&>(request)), C,
        /*download_headers_only=*/
        internal::IsResponseWithoutBodies<typename Endpoint::Response>,
        base::BindOnce(MakeResponse<Response>).Then(std::move(callback)));

    if constexpr (C == RequestCancelability::kCancelable) {
      return RequestHandle(std::move(simple_url_loader),
                           base::SequencedTaskRunner::GetCurrentDefault());
    }
  }

  // Streaming requests, for endpoints whose Response is a Stream<>, are always
  // cancelable. `Stream` defers `Stream::Callbacks` to overload resolution, so
  // that Client<> can still be instantiated for endpoints without a Stream<>.
  template <typename Request, typename Stream = typename Endpoint::Response>
    requires(internal::IsStream<Stream> &&
             std::same_as<internal::MaybeStripWithHeaders<Request>,
                          typename Endpoint::Request>)
  [[nodiscard]] static RequestHandle Send(
      network::mojom::URLLoaderFactory& url_loader_factory,
      Request request,
      typename Stream::Callbacks callbacks) {
    internal::RawStreamCallbacks raw_callbacks;
    raw_callbacks.on_response_started =
        std::move(callbacks.on_response_started);
    raw_callbacks.on_chunk = std::move(callbacks.on_chunk);
    raw_callbacks.on_complete = base::BindOnce(MakeStreamResult<Stream>)
                                    .Then(std::move(callbacks.on_complete));

    auto serialized_request = SerializeRequest(request);
    auto stream_consumer = internal::StartStreamingSimpleURLLoader(
        url_loader_factory, Request::Method(), Endpoint::URL(),
        std::move(serialized_request.headers),
        std::move(serialized_request.upload_data),
        serialized_request.content_type,
        std::move(static_cast<internal::RequestOptions&>(request)),
        /*buffer_error_body=*/internal::IsStreamWithErrorBody<Stream>,
        std::move(raw_callbacks));

    return RequestHandle(std::move(stream_consumer),
                         base::SequencedTaskRunner::GetCurrentDefault());
  }

 private:
  template <typename Request>
  static internal::SerializedRequest SerializeRequest(Request& request) {
    internal::SerializedRequest serialized_request;
    if constexpr (base::is_instantiation<Request, WithHeaders>) {
      serialized_request.headers = std::move(request.headers);
    }
    if constexpr (!internal::IsRequestWithoutBody<typename Endpoint::Request>) {
      serialized_request.upload_data = request.Serialize();
      serialized_request.content_type = Request::ContentType();
    }
    return serialized_request;
  }

  template <typename Stream>
  static typename Stream::Result MakeStreamResult(
      int net_error,
      scoped_refptr<net::HttpResponseHeaders> headers,
      std::optional<std::string> error_body) {
    typename Stream::Result result;
    result.net_error = net_error;
    if (net_error != net::OK || !headers) {
      return result;
    }

    result.status_code = headers->response_code();

    if constexpr (internal::IsStreamWithErrorBody<Stream>) {
      if (error_body) {
        result.error_body =
            internal::DeserializeBody<typename Stream::ErrorBody>(*error_body);
      }
    }

    return result;
  }

  template <typename Response>
  static Response MakeResponse(int net_error,
                               scoped_refptr<net::HttpResponseHeaders> headers,
                               std::optional<std::string> response_body) {
    Response response;
    response.net_error = net_error;
    if (net_error != net::OK || !headers) {
      return response;
    }

    response.status_code = headers->response_code();

    if constexpr (internal::IsResponseWithBodies<typename Endpoint::Response>) {
      // response_body is std::nullopt only if SimpleURLLoader failed or if
      // only headers were downloaded (Response<> without body types).
      // Neither case reaches this point.
      response.body = Response::Deserialize(*response.status_code,
                                            CHECK_DEREF(response_body));
    }

    if constexpr (base::is_instantiation<Response, WithHeaders>) {
      response.headers = std::move(headers);
    }

    return response;
  }
};

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_CLIENT_H_
