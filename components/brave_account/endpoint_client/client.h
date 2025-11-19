/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CLIENT_H_

#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/types/expected.h"
#include "base/types/is_instantiation.h"
#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/is_endpoint.h"
#include "brave/components/brave_account/endpoint_client/maybe_strip_with_headers.h"
#include "brave/components/brave_account/endpoint_client/request_handle.h"
#include "brave/components/brave_account/endpoint_client/response.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "net/base/load_flags.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/header_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

// See //brave/components/brave_account/endpoint_client/README.md
// for design, motivation, usage, and examples.

namespace brave_account::endpoint_client {

enum class RequestCancelability { kNonCancelable, kCancelable };

template <IsEndpoint T>
class Client {
  // Depending on |C|, either takes ownership of (moves out) |simple_url_loader|
  // (non-cancelable case), or returns a raw pointer to the loader (cancelable
  // case).
  template <RequestCancelability C>
  static auto MaybeMoveLoader(
      std::unique_ptr<network::SimpleURLLoader>& simple_url_loader) {
    if constexpr (C == RequestCancelability::kNonCancelable) {
      return std::move(simple_url_loader);
    } else {
      return simple_url_loader.get();
    }
  }

  // Depending on |C|, either verifies that ownership of |simple_url_loader| has
  // already been transferred (non-cancelable case), or wraps the loader in a
  // RequestHandle that safely deletes it on its owning sequence when the handle
  // is reset (cancelable case).
  template <RequestCancelability C>
  static auto MaybeMakeHandle(
      std::unique_ptr<network::SimpleURLLoader>& simple_url_loader) {
    if constexpr (C == RequestCancelability::kNonCancelable) {
      CHECK(!simple_url_loader);
    } else {
      CHECK(simple_url_loader);
      return RequestHandle(simple_url_loader.release(),
                           detail::RequestHandleDeleter(
                               base::SequencedTaskRunner::GetCurrentDefault()));
    }
  }

 public:
  // [[nodiscard]] enforces that callers retain the returned handle in
  // the cancelable case. It has no effect in the non-cancelable case,
  // where the return type is void.
  template <RequestCancelability C = RequestCancelability::kNonCancelable,
            typename Request,
            typename Response>
    requires(std::same_as<detail::MaybeStripWithHeaders<Request>,
                          typename T::Request> &&
             std::same_as<detail::MaybeStripWithHeaders<Response>,
                          typename T::Response>)
  [[nodiscard]] static auto Send(
      const scoped_refptr<network::SharedURLLoaderFactory>& url_loader_factory,
      Request request,
      base::OnceCallback<void(Response)> callback) {
    CHECK(url_loader_factory);
    if (!request.network_traffic_annotation_tag.is_valid()) {
      CHECK_IS_TEST()
          << "Client<> requires a valid network traffic annotation and "
             "only permits a missing annotation in tests.";
      request.network_traffic_annotation_tag =
          net::MutableNetworkTrafficAnnotationTag(MISSING_TRAFFIC_ANNOTATION);
    }

    auto resource_request = std::make_unique<network::ResourceRequest>();
    resource_request->url = T::URL();
    resource_request->method = Request::Method();
    resource_request->load_flags = net::LOAD_BYPASS_CACHE |
                                   net::LOAD_DISABLE_CACHE |
                                   net::LOAD_DO_NOT_SAVE_COOKIES;
    resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
    if constexpr (base::is_instantiation<Request, WithHeaders>) {
      resource_request->headers = std::move(request.headers);
    }

    auto simple_url_loader = network::SimpleURLLoader::Create(
        std::move(resource_request),
        static_cast<net::NetworkTrafficAnnotationTag>(
            request.network_traffic_annotation_tag));
    simple_url_loader->SetAllowHttpErrorResults(true);
    if (const auto dict = request.ToValue(); !dict.empty()) {
      auto json = base::WriteJson(dict).value_or("");
      CHECK(!json.empty()) << "Failed to serialize request to JSON!";
      simple_url_loader->AttachStringForUpload(std::move(json),
                                               "application/json");
    }

    simple_url_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        url_loader_factory.get(),
        base::BindOnce(OnResponse<Response>, std::move(callback),
                       MaybeMoveLoader<C>(simple_url_loader)));

    return MaybeMakeHandle<C>(simple_url_loader);
  }

 private:
  template <typename Response>
  static void OnResponse(
      base::OnceCallback<void(Response)> callback,
      std::variant<std::unique_ptr<network::SimpleURLLoader>,
                   network::SimpleURLLoader*> simple_url_loader,
      std::optional<std::string> response_body) {
    const auto& simple_url_loader_ref = std::visit(
        [](const auto& ptr) -> network::SimpleURLLoader& { return *ptr; },
        simple_url_loader);

    Response response;
    response.net_error = simple_url_loader_ref.NetError();

    auto* const response_info = simple_url_loader_ref.ResponseInfo();
    auto headers = response_info ? response_info->headers : nullptr;
    if (response.net_error != net::OK || !headers) {
      return std::move(callback).Run(std::move(response));
    }

    response.status_code = headers->response_code();
    if constexpr (base::is_instantiation<Response, WithHeaders>) {
      response.headers = std::move(headers);
    }

    const auto value =
        base::JSONReader::Read(response_body.value_or(""), base::JSON_PARSE_RFC)
            .value_or(base::Value());
    // Intentionally kept as an if-else block for symmetry.
    if (network::IsSuccessfulStatus(*response.status_code)) {  // 2xx
      if (auto success_body = Response::SuccessBody::FromValue(value)) {
        response.body = std::move(*success_body);
      }
    } else {  // non-2xx
      if (auto error_body = Response::ErrorBody::FromValue(value)) {
        response.body = base::unexpected(std::move(*error_body));
      }
    }

    std::move(callback).Run(std::move(response));
  }
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CLIENT_H_
