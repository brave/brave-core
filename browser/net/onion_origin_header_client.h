/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_ONION_ORIGIN_HEADER_CLIENT_H_
#define BRAVE_BROWSER_NET_ONION_ORIGIN_HEADER_CLIENT_H_

#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
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
class OnionOriginHeaderClient : public network::mojom::TrustedHeaderClient {
 public:
  // Binds a self-owned client to `receiver`. If `target_client` is valid, the
  // client forwards calls to it after nullifying the Origin header of
  // cross-origin .onion requests.
  static void Create(
      mojo::PendingReceiver<network::mojom::TrustedHeaderClient> receiver,
      mojo::PendingRemote<network::mojom::TrustedHeaderClient> target_client);

  OnionOriginHeaderClient(const OnionOriginHeaderClient&) = delete;
  OnionOriginHeaderClient& operator=(const OnionOriginHeaderClient&) = delete;
  ~OnionOriginHeaderClient() override;

  // network::mojom::TrustedHeaderClient override.
  void OnBeforeSendHeaders(const GURL& request_url,
                           const net::HttpRequestHeaders& headers,
                           OnBeforeSendHeadersCallback callback) override;
  void OnHeadersReceived(const std::string& headers,
                         const net::IPEndPoint& remote_endpoint,
                         const std::optional<net::SSLInfo>& ssl_info,
                         OnHeadersReceivedCallback callback) override;

 private:
  explicit OnionOriginHeaderClient(
      mojo::PendingRemote<network::mojom::TrustedHeaderClient> target_client);

  void OnTargetDisconnect();

  void OnTargetBeforeSendHeadersComplete(
      OnBeforeSendHeadersCallback callback,
      const GURL& request_url,
      const net::HttpRequestHeaders& original_headers,
      int32_t result,
      const std::optional<net::HttpRequestHeaders>& headers,
      std::optional<base::DictValue> extended_net_log_events);

  mojo::Remote<network::mojom::TrustedHeaderClient> target_client_;

  base::WeakPtrFactory<OnionOriginHeaderClient> weak_ptr_factory_{this};
};

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_ONION_ORIGIN_HEADER_CLIENT_H_
