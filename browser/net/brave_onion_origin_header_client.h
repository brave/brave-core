/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_ONION_ORIGIN_HEADER_CLIENT_H_
#define BRAVE_BROWSER_NET_BRAVE_ONION_ORIGIN_HEADER_CLIENT_H_

#include <optional>
#include <string>

#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/ssl/ssl_info.h"
#include "services/network/public/mojom/network_context.mojom.h"

class GURL;

namespace net {
class HttpRequestHeaders;
class IPEndPoint;
}  // namespace net

namespace brave {

// Nullifies the Origin header of cross-origin WebSocket handshakes from .onion
// pages, so they do not disclose the address they came from.
class BraveOnionOriginHeaderClient
    : public network::mojom::TrustedHeaderClient {
 public:
  // Returns a remote to a self-owned client wrapping `inner`, which may be
  // null.
  static mojo::PendingRemote<network::mojom::TrustedHeaderClient> Create(
      mojo::PendingRemote<network::mojom::TrustedHeaderClient> inner);

  explicit BraveOnionOriginHeaderClient(
      mojo::PendingRemote<network::mojom::TrustedHeaderClient> inner);
  BraveOnionOriginHeaderClient(const BraveOnionOriginHeaderClient&) = delete;
  BraveOnionOriginHeaderClient& operator=(const BraveOnionOriginHeaderClient&) =
      delete;
  ~BraveOnionOriginHeaderClient() override;

  // network::mojom::TrustedHeaderClient override.
  void OnBeforeSendHeaders(const GURL& request_url,
                           const net::HttpRequestHeaders& headers,
                           OnBeforeSendHeadersCallback callback) override;
  void OnHeadersReceived(const std::string& headers,
                         const net::IPEndPoint& remote_endpoint,
                         const std::optional<net::SSLInfo>& ssl_info,
                         OnHeadersReceivedCallback callback) override;

 private:
  mojo::Remote<network::mojom::TrustedHeaderClient> inner_;
};

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_BRAVE_ONION_ORIGIN_HEADER_CLIENT_H_
