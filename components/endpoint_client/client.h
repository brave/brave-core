/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_CLIENT_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_CLIENT_H_

#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "base/check.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/endpoint_client/endpoint_builder.h"
#include "brave/components/endpoint_client/parse.h"
#include "brave/components/endpoint_client/with_headers.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace endpoint_client {

template <detail::IsEndpoint Ept>
struct Client {
  template <typename Request, typename Response, typename Error>
    requires(Ept::template kIsRequestSupported<Request> &&
             Ept::template kIsResponseSupportedForRequest<Response, Request> &&
             Ept::template kIsErrorSupportedForRequest<Error, Request>)
  static void Send(
      const scoped_refptr<network::SharedURLLoaderFactory>& url_loader_factory,
      Request request,
      base::OnceCallback<void(base::expected<std::optional<Response>,
                                             std::optional<Error>>)> callback) {
    auto on_response =
        [](decltype(callback) callback,
           std::unique_ptr<network::SimpleURLLoader> simple_url_loader,
           std::optional<std::string> response_body) {
          using Expected =
              base::expected<std::optional<Response>, std::optional<Error>>;

          CHECK(simple_url_loader);

          const auto* response_info = simple_url_loader->ResponseInfo();
          if (!response_info) {
            return std::move(callback).Run(base::unexpected(std::nullopt));
          }

          auto headers = response_info->headers;
          if (!headers) {
            return std::move(callback).Run(base::unexpected(std::nullopt));
          }

          const auto value = base::JSONReader::Read(response_body.value_or(""));
          std::move(callback).Run(
              headers->response_code() / 100 == 2  // 2xx
                  ? Expected(detail::Parse<Response>::From(value,
                                                           std::move(headers)))
                  : base::unexpected(
                        detail::Parse<Error>::From(value, std::move(headers))));
        };

    const auto json = base::WriteJson(request.ToValue());
    CHECK(json) << "Failed to serialize request to JSON!";

    auto resource_request = std::make_unique<network::ResourceRequest>();
    resource_request->url = Ept::URL();
    resource_request->method = request.Method();
    if constexpr (base::is_instantiation<Request, WithHeaders>) {
      resource_request->headers = std::move(request.headers);
    }

    auto simple_url_loader = network::SimpleURLLoader::Create(
        std::move(resource_request),
        net::DefineNetworkTrafficAnnotation("test", "test"));
    simple_url_loader->SetAllowHttpErrorResults(true);
    simple_url_loader->AttachStringForUpload(std::move(*json),
                                             "application/json");

    auto* simple_url_loader_ptr = simple_url_loader.get();
    simple_url_loader_ptr->DownloadToString(
        url_loader_factory.get(),
        base::BindOnce(std::move(on_response), std::move(callback),
                       std::move(simple_url_loader)),
        network::SimpleURLLoader::kMaxBoundedStringDownloadSize);
  }
};

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_CLIENT_H_
