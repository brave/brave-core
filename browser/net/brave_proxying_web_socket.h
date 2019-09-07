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
}

// Ensures that all web socket requests go through Brave network request
// handling framework. Cargoculted from |WebRequestProxyingWebSocket|.
class BraveProxyingWebSocket : public network::mojom::WebSocketHandshakeClient,
                               public network::mojom::AuthenticationHandler,
                               public network::mojom::TrustedHeaderClient {
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
      content::BrowserContext* browser_context,
      scoped_refptr<RequestIDGenerator> request_id_generator,
      BraveRequestHandler* handler,
      DisconnectCallback on_disconnect);
  ~BraveProxyingWebSocket() override;

  static BraveProxyingWebSocket* ProxyWebSocket(
      content::RenderFrameHost* frame,
      content::ContentBrowserClient::WebSocketFactory factory,
      const GURL& url,
      const GURL& site_for_cookies,
      const base::Optional<std::string>& user_agent,
      network::mojom::WebSocketHandshakeClientPtr handshake_client);

  void Start();

  content::ContentBrowserClient::WebSocketFactory web_socket_factory();
  network::mojom::WebSocketHandshakeClientPtrInfo handshake_client();
  bool proxy_has_extra_headers();

  // network::mojom::WebSocketHandshakeClient methods:
  void OnOpeningHandshakeStarted(
      network::mojom::WebSocketHandshakeRequestPtr request) override;
  void OnResponseReceived(
      network::mojom::WebSocketHandshakeResponsePtr response) override;
  void OnConnectionEstablished(network::mojom::WebSocketPtr websocket,
                               const std::string& selected_protocol,
                               const std::string& extensions,
                               uint64_t receive_quota_threshold) override;

  // network::mojom::AuthenticationHandler method:
  void OnAuthRequired(const net::AuthChallengeInfo& auth_info,
                      const scoped_refptr<net::HttpResponseHeaders>& headers,
                      const net::IPEndPoint& remote_endpoint,
                      OnAuthRequiredCallback callback) override;

  // network::mojom::TrustedHeaderClient methods:
  void OnBeforeSendHeaders(const net::HttpRequestHeaders& headers,
                           OnBeforeSendHeadersCallback callback) override;
  void OnHeadersReceived(const std::string& headers,
                         OnHeadersReceivedCallback callback) override;

 private:
  void WebSocketFactoryRun(
      const GURL& url,
      std::vector<network::mojom::HttpHeaderPtr> additional_headers,
      network::mojom::WebSocketHandshakeClientPtr handshake_client,
      network::mojom::AuthenticationHandlerPtr auth_handler,
      network::mojom::TrustedHeaderClientPtr trusted_header_client);

  void OnBeforeSendHeadersComplete(int error_code);
  void OnBeforeRequestComplete(int error_code);
  void ContinueToStartRequest(int error_code);
  void OnHeadersReceivedComplete(int error_code);
  void ContinueToHeadersReceived();
  void OnBeforeSendHeadersCompleteFromProxy(
      int error_code,
      const base::Optional<net::HttpRequestHeaders>& headers);
  void OnHeadersReceivedCompleteFromProxy(
      int error_code,
      const base::Optional<std::string>& headers,
      const GURL& url);

  void PauseIncomingMethodCallProcessing();
  void ResumeIncomingMethodCallProcessing();
  void OnError(int result);
  void OnMojoConnectionError(uint32_t custom_reason,
                             const std::string& description);

  BraveRequestHandler* const request_handler_;
  // TODO(iefremov): Get rid of shared_ptr, we should clearly own the pointer.
  // TODO(iefremov): Init this only once.
  std::shared_ptr<brave::BraveRequestInfo> ctx_;

  const int process_id_;
  const int frame_tree_node_id_;
  content::ContentBrowserClient::WebSocketFactory factory_;
  content::BrowserContext* const browser_context_;
  scoped_refptr<RequestIDGenerator> request_id_generator_;
  network::mojom::WebSocketHandshakeClientPtr forwarding_handshake_client_;
  mojo::Binding<network::mojom::WebSocketHandshakeClient>
      binding_as_handshake_client_;
  mojo::Binding<network::mojom::AuthenticationHandler> binding_as_auth_handler_;
  mojo::Binding<network::mojom::TrustedHeaderClient> binding_as_header_client_;

  network::ResourceRequest request_;
  network::ResourceResponseHead response_;
  scoped_refptr<net::HttpResponseHeaders> override_headers_;

  GURL redirect_url_;
  bool is_done_ = false;
  bool waiting_for_header_client_headers_received_ = false;
  uint64_t request_id_ = 0;

  // chrome websocket proxy
  GURL proxy_url_;
  network::mojom::AuthenticationHandlerPtr proxy_auth_handler_;
  network::mojom::TrustedHeaderClientPtr proxy_trusted_header_client_;

  OnHeadersReceivedCallback on_headers_received_callback_;
  OnBeforeSendHeadersCallback on_before_send_headers_callback_;
  DisconnectCallback on_disconnect_;

  base::WeakPtrFactory<BraveProxyingWebSocket> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(BraveProxyingWebSocket);
};

#endif  // BRAVE_BROWSER_NET_BRAVE_PROXYING_WEB_SOCKET_H_
