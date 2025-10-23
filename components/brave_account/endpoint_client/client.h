/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CLIENT_H_

#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/containers/to_vector.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_account/endpoint_client/concepts.h"
#include "net/base/load_flags.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

// See //brave/components/brave_account/endpoint_client/README.md
// for design, motivation, usage, and examples.

namespace brave_account::endpoint_client {

template <concepts::Request T>
struct WithHeaders : T {
  net::HttpRequestHeaders headers;
};

enum class Cancelability { kNonCancelable, kCancelable };

struct RequestHandleDeleter {
  void operator()(void* p) const {
    delete static_cast<network::SimpleURLLoader*>(p);
  }
};

using RequestHandle = std::unique_ptr<void, RequestHandleDeleter>;

template <concepts::Endpoint T>
class Client {
  using Request = typename T::Request;
  using Response = typename T::Response;
  using Error = typename T::Error;
  using Expected =
      base::expected<std::optional<Response>, std::optional<Error>>;
  using Callback = base::OnceCallback<void(int, Expected)>;

  template <Cancelability C>
  using ReturnType =
      std::conditional_t<C == Cancelability::kCancelable, RequestHandle, void>;

 public:
  template <Cancelability C = Cancelability::kNonCancelable>
  static ReturnType<C> Send(
      const scoped_refptr<network::SharedURLLoaderFactory>& url_loader_factory,
      Request request,
      Callback callback) {
    return SendImpl<C>(url_loader_factory, std::move(request),
                       net::HttpRequestHeaders(), std::move(callback));
  }

  template <Cancelability C = Cancelability::kNonCancelable>
  static ReturnType<C> Send(
      const scoped_refptr<network::SharedURLLoaderFactory>& url_loader_factory,
      WithHeaders<Request> request,
      Callback callback) {
    auto headers = std::move(request.headers);
    return SendImpl<C>(url_loader_factory, std::move(request),
                       std::move(headers), std::move(callback));
  }

 private:
  template <Cancelability C>
  using SimpleURLLoaderPtr =
      std::conditional_t<C == Cancelability::kNonCancelable,
                         std::unique_ptr<network::SimpleURLLoader>,
                         base::WeakPtr<const network::SimpleURLLoader>>;

  template <Cancelability C>
  static SimpleURLLoaderPtr<C> MakeSimpleURLLoaderPtr(
      std::unique_ptr<network::SimpleURLLoader>& loader) {
    if constexpr (C == Cancelability::kCancelable) {
      return loader->GetWeakPtr();
    } else {
      return std::exchange(loader, nullptr);
    }
  }

  template <Cancelability C>
  static ReturnType<C> Return(
      std::unique_ptr<network::SimpleURLLoader> simple_url_loader) {
    if constexpr (C == Cancelability::kCancelable) {
      CHECK(simple_url_loader);
      return RequestHandle(simple_url_loader.release());
    } else {
      CHECK(!simple_url_loader);
    }
  }

  template <Cancelability C>
  static ReturnType<C> SendImpl(
      const scoped_refptr<network::SharedURLLoaderFactory>& url_loader_factory,
      Request request,
      net::HttpRequestHeaders headers,
      Callback callback) {
    auto on_response = [](Callback callback,
                          SimpleURLLoaderPtr<C> simple_url_loader_ptr,
                          std::optional<std::string> response_body) {
      const auto* simple_url_loader =
          simple_url_loader_ptr ? simple_url_loader_ptr.get() : nullptr;

      if constexpr (C == Cancelability::kCancelable) {
        if (!simple_url_loader) {
          return;
        }
      }

      const auto* response_info = CHECK_DEREF(simple_url_loader).ResponseInfo();
      if (!response_info) {
        return std::move(callback).Run(-1, base::unexpected(std::nullopt));
      }

      auto headers = response_info->headers;
      if (!headers) {
        return std::move(callback).Run(-1, base::unexpected(std::nullopt));
      }

      const auto value = base::JSONReader::Read(response_body.value_or(""));
      if (!value) {
        return std::move(callback).Run(headers->response_code(),
                                       base::unexpected(std::nullopt));
      }

      DVLOG(0) << "Response: " << *value;
      std::move(callback).Run(headers->response_code(),
                              headers->response_code() / 100 == 2  // 2xx
                                  ? Expected(Response::FromValue(*value))
                                  : base::unexpected(Error::FromValue(*value)));
    };

    std::string json;
    if (auto dict = request.ToValue(); !dict.empty()) {
      json = base::WriteJson(std::move(dict)).value_or("");
      CHECK(!json.empty()) << "Failed to serialize request to JSON!";
    }

    auto resource_request = std::make_unique<network::ResourceRequest>();
    resource_request->url = T::URL();
    resource_request->method = T::Method();
    resource_request->headers = std::move(headers);
    resource_request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES |
                                   net::LOAD_BYPASS_CACHE |
                                   net::LOAD_DISABLE_CACHE;
    resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;

    auto simple_url_loader = network::SimpleURLLoader::Create(
        std::move(resource_request),
        net::DefineNetworkTrafficAnnotation("test", "test"));
    simple_url_loader->SetAllowHttpErrorResults(true);
    if (!json.empty()) {
      simple_url_loader->AttachStringForUpload(std::move(json),
                                               "application/json");
    }

    auto* simple_url_loader_raw = simple_url_loader.get();
    simple_url_loader_raw->DownloadToString(
        url_loader_factory.get(),
        base::BindOnce(std::move(on_response), std::move(callback),
                       MakeSimpleURLLoaderPtr<C>(simple_url_loader)),
        network::SimpleURLLoader::kMaxBoundedStringDownloadSize);

    return Return<C>(std::move(simple_url_loader));
  }
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_CLIENT_H_
