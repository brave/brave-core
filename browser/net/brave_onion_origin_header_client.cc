/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_onion_origin_header_client.h"

#include <memory>
#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/values.h"
#include "mojo/public/cpp/bindings/callback_helpers.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/base/ip_endpoint.h"
#include "net/base/net_errors.h"
#include "net/base/url_util.h"
#include "net/http/http_request_headers.h"
#include "net/ssl/ssl_info.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace brave {

namespace {

net::HttpRequestHeaders HideCrossOriginOnionOrigin(
    const GURL& url,
    net::HttpRequestHeaders headers) {
  std::optional<std::string> origin =
      headers.GetHeader(net::HttpRequestHeaders::kOrigin);
  if (origin &&
      net::IsCrossOriginOnionRequest(url::Origin::Create(GURL(*origin)), url)) {
    headers.SetHeader(net::HttpRequestHeaders::kOrigin,
                      url::Origin().Serialize());
  }
  return headers;
}

}  // namespace

// static
mojo::PendingRemote<network::mojom::TrustedHeaderClient>
BraveOnionOriginHeaderClient::Create(
    mojo::PendingRemote<network::mojom::TrustedHeaderClient> inner) {
  mojo::PendingRemote<network::mojom::TrustedHeaderClient> remote;
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<BraveOnionOriginHeaderClient>(std::move(inner)),
      remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

BraveOnionOriginHeaderClient::BraveOnionOriginHeaderClient(
    mojo::PendingRemote<network::mojom::TrustedHeaderClient> inner) {
  if (inner) {
    inner_.Bind(std::move(inner));
  }
}

BraveOnionOriginHeaderClient::~BraveOnionOriginHeaderClient() = default;

void BraveOnionOriginHeaderClient::OnBeforeSendHeaders(
    const GURL& request_url,
    const net::HttpRequestHeaders& headers,
    OnBeforeSendHeadersCallback callback) {
  if (!inner_) {
    std::move(callback).Run(net::OK,
                            HideCrossOriginOnionOrigin(request_url, headers),
                            std::nullopt);
    return;
  }

  // If `inner_` disconnects, fail the request rather than leaving it hanging.
  inner_->OnBeforeSendHeaders(
      request_url, headers,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          base::BindOnce(
              [](const GURL& request_url,
                 const net::HttpRequestHeaders& original_headers,
                 OnBeforeSendHeadersCallback callback, int32_t result,
                 const std::optional<net::HttpRequestHeaders>& headers,
                 std::optional<base::DictValue> extended_net_log_events) {
                std::move(callback).Run(
                    result,
                    HideCrossOriginOnionOrigin(
                        request_url, headers.value_or(original_headers)),
                    std::move(extended_net_log_events));
              },
              request_url, headers, std::move(callback)),
          net::ERR_FAILED, std::nullopt, std::nullopt));
}

void BraveOnionOriginHeaderClient::OnHeadersReceived(
    const std::string& headers,
    const net::IPEndPoint& remote_endpoint,
    const std::optional<net::SSLInfo>& ssl_info,
    OnHeadersReceivedCallback callback) {
  if (!inner_) {
    std::move(callback).Run(net::OK, std::nullopt, std::nullopt);
    return;
  }

  inner_->OnHeadersReceived(
      headers, remote_endpoint, ssl_info,
      mojo::WrapCallbackWithDefaultInvokeIfNotRun(
          std::move(callback), net::ERR_FAILED, std::nullopt, std::nullopt));
}

}  // namespace brave
