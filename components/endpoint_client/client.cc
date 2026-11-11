/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/endpoint_client/client.h"

#include <utility>

#include "base/check_is_test.h"
#include "net/base/load_flags.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/mojom/fetch_api.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace endpoint_client::internal {

namespace {

void OnBodyAsString(OnResponseCallback on_response_callback,
                    network::SimpleURLLoader* simple_url_loader,
                    std::optional<std::string> response_body) {
  auto response_info = simple_url_loader->TakeResponseInfo();
  std::move(on_response_callback)
      .Run(simple_url_loader->NetError(),
           response_info ? std::move(response_info->headers) : nullptr,
           std::move(response_body));
}

}  // namespace

std::unique_ptr<network::SimpleURLLoader> StartSimpleURLLoader(
    network::mojom::URLLoaderFactory& url_loader_factory,
    std::string_view method,
    const GURL& url,
    net::HttpRequestHeaders headers,
    std::optional<std::string> upload_data,
    std::string_view content_type,
    RequestOptions options,
    RequestCancelability cancelability,
    OnResponseCallback on_response_callback) {
  if (!options.network_traffic_annotation_tag.is_valid()) {
    CHECK_IS_TEST() << "Client<> requires a valid network traffic annotation "
                       "and only permits a missing annotation in tests.";
    options.network_traffic_annotation_tag =
        net::MutableNetworkTrafficAnnotationTag(MISSING_TRAFFIC_ANNOTATION);
  }

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->method = method;
  resource_request->url = options.url_replacements.Apply(url);
  resource_request->headers = std::move(headers);
  resource_request->load_flags =
      net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;
  // kOmit implies net::LOAD_DO_NOT_SAVE_COOKIES (deprecated) via
  // URLRequest::set_disallow_credentials(); weaker modes save cookies again.
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->redirect_mode = options.redirect_mode;

  auto simple_url_loader = network::SimpleURLLoader::Create(
      std::move(resource_request),
      static_cast<net::NetworkTrafficAnnotationTag>(
          options.network_traffic_annotation_tag));
  simple_url_loader->SetAllowHttpErrorResults(true);
  if (upload_data) {
    simple_url_loader->AttachStringForUpload(std::move(*upload_data),
                                             content_type);
  }
  simple_url_loader->SetRetryOptions(options.retry_options.max_retries,
                                     options.retry_options.retry_mode);
  simple_url_loader->SetTimeoutDuration(options.timeout_duration);

  auto* simple_url_loader_ptr = simple_url_loader.get();
  // How the response callback holds the loader.
  // Cancelable: `RequestHandle` owns the loader, so `base::Unretained<>()`
  // binds a non-owning pointer to it, which can't dangle when the callback
  // runs, since only the loader runs the callback.
  // Non-cancelable: `base::Owned<>()` transfers ownership to the callback;
  // the loader is deleted when the callback is.
  auto body_as_string_callback =
      cancelability == RequestCancelability::kCancelable
          ? base::BindOnce(OnBodyAsString, std::move(on_response_callback),
                           base::Unretained(simple_url_loader_ptr))
          : base::BindOnce(OnBodyAsString, std::move(on_response_callback),
                           base::Owned(std::move(simple_url_loader)));

  if (options.max_body_size) {
    simple_url_loader_ptr->DownloadToString(&url_loader_factory,
                                            std::move(body_as_string_callback),
                                            *options.max_body_size);
  } else {
    simple_url_loader_ptr->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        &url_loader_factory, std::move(body_as_string_callback));
  }

  return simple_url_loader;
}

}  // namespace endpoint_client::internal
