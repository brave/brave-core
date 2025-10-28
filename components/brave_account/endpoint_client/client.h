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
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
#include "base/types/is_instantiation.h"
#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/is_endpoint.h"
#include "brave/components/brave_account/endpoint_client/maybe_strip_with_headers.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

// See //brave/components/brave_account/endpoint_client/README.md
// for design, motivation, usage, and examples.

namespace brave_account::endpoint_client {

template <IsEndpoint T>
class Client {
  using Response = typename T::Response;
  using Error = typename T::Error;
  using Expected =
      base::expected<std::optional<Response>, std::optional<Error>>;
  using Callback = base::OnceCallback<void(int, Expected)>;

 public:
  template <typename Request>
    requires(std::same_as<detail::MaybeStripWithHeaders<Request>,
                          typename T::Request>)
  static void Send(
      const scoped_refptr<network::SharedURLLoaderFactory>& url_loader_factory,
      Request request,
      Callback callback) {
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
        base::BindOnce(OnResponse, std::move(callback),
                       std::move(simple_url_loader)));
  }

 private:
  static void OnResponse(
      Callback callback,
      std::unique_ptr<network::SimpleURLLoader> simple_url_loader,
      std::optional<std::string> response_body) {
    auto* const response_info = simple_url_loader->ResponseInfo();
    const auto headers = response_info ? response_info->headers : nullptr;
    if (!headers) {
      return std::move(callback).Run(-1, base::unexpected(std::nullopt));
    }

    const auto response_code = headers->response_code();
    const auto value =
        base::JSONReader::Read(response_body.value_or(""), base::JSON_PARSE_RFC)
            .value_or(base::Value());
    std::move(callback).Run(response_code,
                            response_code / 100 == 2  // 2xx
                                ? Expected(Response::FromValue(value))
                                : base::unexpected(Error::FromValue(value)));
  }
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CLIENT_H_
