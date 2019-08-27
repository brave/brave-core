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
    : public network::mojom::WebSocket,
      public network::mojom::WebSocketClient {
 public:
  using DisconnectCallback =
      base::OnceCallback<void(BraveProxyingWebSocket*)>;

  BraveProxyingWebSocket(
      BraveRequestHandler* handler,
      content::ResourceContext* resource_context,
      int process_id,
      int frame_id,
      int frame_tree_node_id,
      const url::Origin& origin,
      scoped_refptr<RequestIDGenerator> request_id_generator,
      network::mojom::WebSocketPtr proxied_socket,
      network::mojom::WebSocketRequest proxied_request,
      DisconnectCallback on_disconnect);
  ~BraveProxyingWebSocket() override;

  static bool ProxyWebSocket(
      content::RenderFrameHost* frame,
      network::mojom::WebSocketRequest* request,
      network::mojom::AuthenticationHandlerPtr* auth_handler);

  // mojom::WebSocket methods:
  void AddChannelRequest(
      const GURL& url,
      const std::vector<std::string>& requested_protocols,
      const GURL& site_for_cookies,
      std::vector<network::mojom::HttpHeaderPtr> additional_headers,
      network::mojom::WebSocketClientPtr client) override;
  void SendFrame(bool fin,
                 network::mojom::WebSocketMessageType type,
                 const std::vector<uint8_t>& data) override;
  void AddReceiveFlowControlQuota(int64_t quota) override;
  void StartClosingHandshake(uint16_t code, const std::string& reason) override;

  // mojom::WebSocketClient methods:
  void OnFailChannel(const std::string& reason) override;
  void OnStartOpeningHandshake(
      network::mojom::WebSocketHandshakeRequestPtr request) override;
  void OnFinishOpeningHandshake(
      network::mojom::WebSocketHandshakeResponsePtr response) override;
  void OnAddChannelResponse(const std::string& selected_protocol,
                            const std::string& extensions) override;
  void OnDataFrame(bool fin,
                   network::mojom::WebSocketMessageType type,
                   const std::vector<uint8_t>& data) override;
  void OnFlowControl(int64_t quota) override;
  void OnDropChannel(bool was_clean,
                     uint16_t code,
                     const std::string& reason) override;
  void OnClosingHandshake() override;

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
  const int frame_id_;
  const int frame_tree_node_id_;
  const url::Origin origin_;
  content::ResourceContext* const resource_context_;
  scoped_refptr<RequestIDGenerator> request_id_generator_;
  network::mojom::WebSocketPtr proxied_socket_;
  network::mojom::WebSocketClientPtr forwarding_client_;
  mojo::Binding<network::mojom::WebSocket> binding_as_websocket_{this};
  mojo::Binding<network::mojom::WebSocketClient> binding_as_client_{this};

  network::ResourceRequest request_;
  network::ResourceResponseHead response_;
  scoped_refptr<net::HttpResponseHeaders> override_headers_;
  std::vector<std::string> websocket_protocols_;
  std::vector<network::mojom::HttpHeaderPtr> additional_headers_;

  GURL redirect_url_;
  bool is_done_ = false;
  uint64_t request_id_ = 0;

  DisconnectCallback on_disconnect_;

  base::WeakPtrFactory<BraveProxyingWebSocket> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(BraveProxyingWebSocket);
};

#endif  // BRAVE_BROWSER_NET_BRAVE_PROXYING_WEB_SOCKET_H_
