/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_BRAVE_PROXYING_WEB_SOCKET_H_
#define BRAVE_BROWSER_NET_BRAVE_PROXYING_WEB_SOCKET_H_

#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "brave/browser/net/resource_context_data.h"
#include "brave/browser/net/url_context.h"
#include "content/public/browser/content_browser_client.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "mojo/public/cpp/bindings/binding_set.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/websocket.mojom.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {
class BrowserContext;
class RenderFrameHost;
class ResourceContext;
}

// Ensures that all web socket requests go through Brave network request
// handling framework. Cargoculted from |WebRequestProxyingWebSocket|.
class BraveProxyingWebSocket
    : public network::mojom::WebSocketHandshakeClient {
 public:
  using WebSocketFactory = content::ContentBrowserClient::WebSocketFactory;
  using DisconnectCallback =
      base::OnceCallback<void(BraveProxyingWebSocket*)>;

  BraveProxyingWebSocket(
      WebSocketFactory factory,
      const network::ResourceRequest& request,
      network::mojom::WebSocketHandshakeClientPtr handshake_client,
      int process_id,
      int frame_tree_node_id,
      content::ResourceContext* resource_context,
      scoped_refptr<RequestIDGenerator> request_id_generator,
      BraveRequestHandler* handler,
      DisconnectCallback on_disconnect);
  ~BraveProxyingWebSocket() override;

  static bool ProxyWebSocket(
      content::RenderFrameHost* frame,
      content::ContentBrowserClient::WebSocketFactory factory,
      const GURL& url,
      const GURL& site_for_cookies,
      const base::Optional<std::string>& user_agent,
      network::mojom::WebSocketHandshakeClientPtr handshake_client);

  void Start();

  // network::mojom::WebSocketHandShakeClient methods:
  void OnOpeningHandshakeStarted(
      network::mojom::WebSocketHandshakeRequestPtr request) override;
  void OnResponseReceived(
      network::mojom::WebSocketHandshakeResponsePtr response) override;
  void OnConnectionEstablished(network::mojom::WebSocketPtr websocket,
                               const std::string& selected_protocol,
                               const std::string& extensions,
                               uint64_t receive_quota_threshold) override;

 private:
  void OnBeforeRequestComplete(int error_code);
  void ContinueToStartRequest(int error_code);
  void OnHeadersReceivedComplete(int error_code);
  void ContinueToHeadersReceived();

  void PauseIncomingMethodCallProcessing();
  void ResumeIncomingMethodCallProcessing();
  void OnError(int result);

  BraveRequestHandler* const request_handler_;
  // TODO(iefremov): Get rid of shared_ptr, we should clearly own the pointer.
  // TODO(iefremov): Init this only once.
  std::shared_ptr<brave::BraveRequestInfo> ctx_;

  const int process_id_;
  const int frame_tree_node_id_;
  content::ContentBrowserClient::WebSocketFactory factory_;
  content::ResourceContext* const resource_context_;
  scoped_refptr<RequestIDGenerator> request_id_generator_;
  network::mojom::WebSocketHandshakeClientPtr forwarding_handshake_client_;
  mojo::Binding<network::mojom::WebSocketHandshakeClient>
      binding_as_handshake_client_{this};

  network::ResourceRequest request_;
  network::ResourceResponseHead response_;
  scoped_refptr<net::HttpResponseHeaders> override_headers_;
  std::vector<network::mojom::HttpHeaderPtr> additional_headers_;

  GURL redirect_url_;
  bool is_done_ = false;
  uint64_t request_id_ = 0;

  DisconnectCallback on_disconnect_;

  base::WeakPtrFactory<BraveProxyingWebSocket> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(BraveProxyingWebSocket);
};

#endif  // BRAVE_BROWSER_NET_BRAVE_PROXYING_WEB_SOCKET_H_
