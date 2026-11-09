/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/onion_origin_header_client.h"

#include <optional>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
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

net::HttpRequestHeaders MaybeHideCrossOriginOnionOrigin(
    const GURL& url,
    net::HttpRequestHeaders headers) {
  std::optional<std::string> origin =
      headers.GetHeader(net::HttpRequestHeaders::kOrigin);
  if (origin && net::IsCrossOriginOnionRequest(*origin, url)) {
    headers.SetHeader(net::HttpRequestHeaders::kOrigin,
                      url::Origin().Serialize());
  }
  return headers;
}

}  // namespace

// static
void OnionOriginHeaderClient::Create(
    mojo::PendingReceiver<network::mojom::TrustedHeaderClient> receiver,
    mojo::PendingRemote<network::mojom::TrustedHeaderClient> target_client) {
  mojo::MakeSelfOwnedReceiver(
      base::WrapUnique(new OnionOriginHeaderClient(std::move(target_client))),
      std::move(receiver));
}

OnionOriginHeaderClient::OnionOriginHeaderClient(
    mojo::PendingRemote<network::mojom::TrustedHeaderClient> target_client) {
  if (target_client) {
    target_client_.Bind(std::move(target_client));
    target_client_.set_disconnect_handler(base::BindOnce(
        &OnionOriginHeaderClient::OnTargetDisconnect, base::Unretained(this)));
  }
}

OnionOriginHeaderClient::~OnionOriginHeaderClient() = default;

void OnionOriginHeaderClient::OnBeforeSendHeaders(
    const GURL& request_url,
    const net::HttpRequestHeaders& headers,
    OnBeforeSendHeadersCallback callback) {
  if (target_client_.is_bound()) {
    target_client_->OnBeforeSendHeaders(
        request_url, headers,
        mojo::WrapCallbackWithDefaultInvokeIfNotRun(
            base::BindOnce(
                &OnionOriginHeaderClient::OnTargetBeforeSendHeadersComplete,
                weak_ptr_factory_.GetWeakPtr(), std::move(callback),
                request_url, headers),
            net::ERR_FAILED, std::nullopt, std::nullopt));
  } else {
    OnTargetBeforeSendHeadersComplete(std::move(callback), request_url, headers,
                                      net::OK, std::nullopt, std::nullopt);
  }
}

void OnionOriginHeaderClient::OnHeadersReceived(
    const std::string& headers,
    const net::IPEndPoint& remote_endpoint,
    const std::optional<net::SSLInfo>& ssl_info,
    OnHeadersReceivedCallback callback) {
  if (target_client_.is_bound()) {
    target_client_->OnHeadersReceived(
        headers, remote_endpoint, ssl_info,
        mojo::WrapCallbackWithDefaultInvokeIfNotRun(
            std::move(callback), net::ERR_FAILED, std::nullopt, std::nullopt));
  } else {
    std::move(callback).Run(net::OK, std::nullopt, std::nullopt);
  }
}

void OnionOriginHeaderClient::OnTargetDisconnect() {
  target_client_.reset();
}

void OnionOriginHeaderClient::OnTargetBeforeSendHeadersComplete(
    OnBeforeSendHeadersCallback callback,
    const GURL& request_url,
    const net::HttpRequestHeaders& original_headers,
    int32_t result,
    const std::optional<net::HttpRequestHeaders>& headers,
    std::optional<base::DictValue> extended_net_log_events) {
  std::move(callback).Run(result,
                          MaybeHideCrossOriginOnionOrigin(
                              request_url, headers.value_or(original_headers)),
                          std::move(extended_net_log_events));
}

}  // namespace brave
