/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_CLIENT_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_CLIENT_H_

#include <optional>
#include <string>
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
#include "net/http/http_request_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

// See //brave/components/endpoint_client/README.md
// for design, motivation, usage, and examples.

namespace endpoint_client {

template <endpoints::concepts::Endpoint Endpoint>
class Client {
  template <typename T>
  struct Parse {
    static auto FromValue(const base::Value& value) {
      return T::FromValue(value);
    }
  };

  template <typename... Ts>
  struct Parse<std::variant<Ts...>> {
    static auto FromValue(const base::Value& value) {
      std::optional<std::variant<Ts...>> result;
      (
          [&] {
            if (result) {
              return;
            }

            if (auto t = Ts::FromValue(value)) {
              result = std::move(*t);
            }
          }(),
          ...);
      return result;
    }
  };

 public:
  template <endpoints::concepts::SupportedBy<Endpoint> Request>
  static void Send(
      const scoped_refptr<network::SharedURLLoaderFactory>& url_loader_factory,
      Request request,
      Endpoint::template CallbackFor<Request> callback) {
    auto on_response =
        [](decltype(callback) callback,
           std::unique_ptr<network::SimpleURLLoader> simple_url_loader,
           std::optional<std::string> response_body) {
          using Response = Endpoint::template ResponseFor<Request>;
          using Error = Endpoint::template ErrorFor<Request>;
          using Expected = Endpoint::template ExpectedFor<Request>;

          CHECK(simple_url_loader);

          const auto* response_info = simple_url_loader->ResponseInfo();
          if (!response_info) {
            return std::move(callback).Run(simple_url_loader->NetError(),
                                           base::unexpected(std::nullopt));
          }

          const auto& headers = response_info->headers;
          if (!headers) {
            return std::move(callback).Run(simple_url_loader->NetError(),
                                           base::unexpected(std::nullopt));
          }

          const auto status_code = headers->response_code();

          const auto reply = base::JSONReader::Read(response_body.value_or(""));
          std::move(callback).Run(
              status_code,
              status_code >= 200 && status_code < 300
                  ? Expected(reply ? Parse<Response>::FromValue(*reply)
                                   : std::nullopt)
                  : base::unexpected(reply ? Parse<Error>::FromValue(*reply)
                                           : std::nullopt));
        };

    const auto json = base::WriteJson(request.ToValue());
    CHECK(json) << "Failed to serialize request to JSON!";

    auto resource_request = std::make_unique<network::ResourceRequest>();
    resource_request->url = Endpoint::URL();
    resource_request->method = request.Method();
    resource_request->headers = std::move(request.headers);

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
