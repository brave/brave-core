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

#include "base/check.h"
#include "base/check_deref.h"
#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/types/is_instantiation.h"
#include "brave/components/brave_account/endpoint_client/is_endpoint.h"
#include "brave/components/brave_account/endpoint_client/maybe_strip_with_headers.h"
#include "brave/components/brave_account/endpoint_client/request_handle.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

// See //brave/components/brave_account/endpoint_client/README.md
// for design, motivation, usage, and examples.

namespace brave_account::endpoint_client {

enum class RequestCancelability { kNonCancelable, kCancelable };

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
    requires(std::same_as<detail::MaybeStripWithHeaders<Request>,
                          typename Endpoint::Request> &&
             std::same_as<detail::MaybeStripWithHeaders<Response>,
                          typename Endpoint::Response>)
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
    resource_request->url = request.url_replacements.Apply(Endpoint::URL());
    resource_request->method = Request::Method();
    resource_request->load_flags =
        net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;
    // kOmit implies net::LOAD_DO_NOT_SAVE_COOKIES (deprecated) via
    // URLRequest::set_disallow_credentials(); weaker modes save cookies again.
    resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
    resource_request->redirect_mode = request.redirect_mode;
    if constexpr (base::is_instantiation<Request, WithHeaders>) {
      resource_request->headers = std::move(request.headers);
    }

    auto simple_url_loader = network::SimpleURLLoader::Create(
        std::move(resource_request),
        static_cast<net::NetworkTrafficAnnotationTag>(
            request.network_traffic_annotation_tag));
    simple_url_loader->SetAllowHttpErrorResults(true);
    if (auto upload_data = request.Serialize()) {
      simple_url_loader->AttachStringForUpload(std::move(*upload_data),
                                               Request::ContentType());
    }
    simple_url_loader->SetRetryOptions(request.retry_options.max_retries,
                                       request.retry_options.retry_mode);
    simple_url_loader->SetTimeoutDuration(request.timeout_duration);

    auto* simple_url_loader_ptr = simple_url_loader.get();
    // How the response callback holds the loader.
    // Cancelable: `RequestHandle` owns the loader, so `base::Unretained<>()`
    // binds a non-owning pointer to it, which can't dangle when the callback
    // runs, since only the loader runs the callback.
    // Non-cancelable: `base::Owned<>()` transfers ownership to the callback;
    // the loader is deleted when the callback is.
    auto bound_simple_url_loader = [&] {
      if constexpr (C == RequestCancelability::kCancelable) {
        return base::Unretained(simple_url_loader_ptr);
      } else {
        return base::Owned(std::move(simple_url_loader));
      }
    }();

    auto on_response = base::BindOnce(OnResponse<Response>, std::move(callback),
                                      std::move(bound_simple_url_loader));
    if (request.max_body_size) {
      simple_url_loader_ptr->DownloadToString(url_loader_factory.get(),
                                              std::move(on_response),
                                              *request.max_body_size);
    } else {
      simple_url_loader_ptr->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
          url_loader_factory.get(), std::move(on_response));
    }

    if constexpr (C == RequestCancelability::kCancelable) {
      return RequestHandle(std::move(simple_url_loader),
                           base::SequencedTaskRunner::GetCurrentDefault());
    }
  }

 private:
  template <typename Response>
  static void OnResponse(base::OnceCallback<void(Response)> callback,
                         const network::SimpleURLLoader* simple_url_loader_ptr,
                         std::optional<std::string> response_body) {
    const network::SimpleURLLoader& simple_url_loader =
        CHECK_DEREF(simple_url_loader_ptr);

    auto* response_info = simple_url_loader.ResponseInfo();
    auto headers = response_info ? response_info->headers : nullptr;

    Response response(Response::Deserialize(simple_url_loader.NetError(),
                                            headers, std::move(response_body)));
    if constexpr (base::is_instantiation<Response, WithHeaders>) {
      response.headers = std::move(headers);
    }

    std::move(callback).Run(std::move(response));
  }
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CLIENT_H_
