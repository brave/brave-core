/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_proxying_web_socket.h"

#include <utility>

#include "base/containers/flat_set.h"
#include "base/bind.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/task/post_task.h"
#include "brave/browser/net/brave_request_handler.h"
#include "brave/common/network_constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"

BraveProxyingWebSocket::BraveProxyingWebSocket(
    WebSocketFactory factory,
    const network::ResourceRequest& request,
    network::mojom::WebSocketHandshakeClientPtr handshake_client,
    int process_id,
    int frame_tree_node_id,
    content::BrowserContext* browser_context,
    scoped_refptr<RequestIDGenerator> request_id_generator,
    BraveRequestHandler* handler,
    DisconnectCallback on_disconnect)
    : request_handler_(handler),
      process_id_(process_id),
      frame_tree_node_id_(frame_tree_node_id),
      factory_(std::move(factory)),
      browser_context_(browser_context),
      request_id_generator_(std::move(request_id_generator)),
      forwarding_handshake_client_(std::move(handshake_client)),
      binding_as_handshake_client_(this),
      binding_as_auth_handler_(this),
      binding_as_header_client_(this),
      request_(request),
      on_disconnect_(std::move(on_disconnect)) {}

BraveProxyingWebSocket::~BraveProxyingWebSocket() {
  if (ctx_) {
    request_handler_->OnURLRequestDestroyed(ctx_);
  }
}

// static
BraveProxyingWebSocket* BraveProxyingWebSocket::ProxyWebSocket(
    content::RenderFrameHost* frame,
    content::ContentBrowserClient::WebSocketFactory factory,
    const GURL& url,
    const GURL& site_for_cookies,
    const base::Optional<std::string>& user_agent,
    network::mojom::WebSocketHandshakeClientPtr handshake_client) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  return ResourceContextData::StartProxyingWebSocket(
      std::move(factory), url, site_for_cookies, user_agent,
      handshake_client.PassInterface(),
      frame->GetProcess()->GetBrowserContext(),
      frame->GetProcess()->GetID(), frame->GetRoutingID(),
      frame->GetFrameTreeNodeId(), frame->GetLastCommittedOrigin());
}

void BraveProxyingWebSocket::Start() {
  request_id_ = request_id_generator_->Generate();
  // If the header client will be used, we start the request immediately, and
  // OnBeforeSendHeaders and OnSendHeaders will be handled there. Otherwise,
  // send these events before the request starts.
  base::RepeatingCallback<void(int)> continuation;
  if (proxy_has_extra_headers()) {
    continuation = base::BindRepeating(
        &BraveProxyingWebSocket::ContinueToStartRequest,
        weak_factory_.GetWeakPtr());
  } else {
    continuation = base::BindRepeating(
        &BraveProxyingWebSocket::OnBeforeRequestComplete,
        weak_factory_.GetWeakPtr());
  }

  ctx_ = std::make_shared<brave::BraveRequestInfo>();
  brave::BraveRequestInfo::FillCTX(request_, process_id_,
                                   frame_tree_node_id_, request_id_,
                                   browser_context_, ctx_);
  int result = request_handler_->OnBeforeURLRequest(
      ctx_, continuation, &redirect_url_);
  // TODO(bridiver) - need to handle general case for redirect_url

  if (result == net::ERR_BLOCKED_BY_CLIENT ||
      // handle adblock kEmptyDataURI
      redirect_url_ == kEmptyDataURI) {
    OnError(result);
    return;
  }

  if (result == net::ERR_IO_PENDING) {
    return;
  }

  DCHECK_EQ(net::OK, result);
  continuation.Run(net::OK);
}

content::ContentBrowserClient::WebSocketFactory
BraveProxyingWebSocket::web_socket_factory() {
  return base::BindOnce(&BraveProxyingWebSocket::WebSocketFactoryRun,
                        base::Unretained(this));
}

network::mojom::WebSocketHandshakeClientPtrInfo
BraveProxyingWebSocket::handshake_client() {
  return forwarding_handshake_client_.PassInterface();
}

bool BraveProxyingWebSocket::proxy_has_extra_headers() {
  return proxy_trusted_header_client_.is_bound();
}

void BraveProxyingWebSocket::WebSocketFactoryRun(const GURL& url,
      std::vector<network::mojom::HttpHeaderPtr> additional_headers,
      network::mojom::WebSocketHandshakeClientPtr handshake_client,
      network::mojom::AuthenticationHandlerPtr auth_handler,
      network::mojom::TrustedHeaderClientPtr trusted_header_client) {
  DCHECK(!forwarding_handshake_client_);
  proxy_url_ = url;
  forwarding_handshake_client_ = std::move(handshake_client);
  proxy_auth_handler_ = std::move(auth_handler);
  proxy_trusted_header_client_ = std::move(trusted_header_client);

  if (!proxy_has_extra_headers()) {
    for (const auto& header : additional_headers) {
      request_.headers.SetHeader(header->name, header->value);
    }
  }

  Start();
}

void BraveProxyingWebSocket::OnOpeningHandshakeStarted(
    network::mojom::WebSocketHandshakeRequestPtr request) {
  DCHECK(forwarding_handshake_client_);
  forwarding_handshake_client_->OnOpeningHandshakeStarted(std::move(request));
}

void BraveProxyingWebSocket::OnResponseReceived(
    network::mojom::WebSocketHandshakeResponsePtr response) {
  // response_.headers will be set in OnBeforeSendHeaders if
  // proxy_has_extra_headers() is set.
  if (!proxy_has_extra_headers()) {
    response_.headers =
        base::MakeRefCounted<net::HttpResponseHeaders>(base::StringPrintf(
            "HTTP/%d.%d %d %s", response->http_version.major_value(),
            response->http_version.minor_value(), response->status_code,
            response->status_text.c_str()));
    for (const auto& header : response->headers)
      response_.headers->AddHeader(header->name + ": " + header->value);
  }

  response_.remote_endpoint = response->remote_endpoint;

  // TODO(yhirano): OnResponseReceived is called with the original
  // response headers. That means if OnHeadersReceived modified them the
  // renderer won't see that modification. This is the opposite of http(s)
  // requests.
  forwarding_handshake_client_->OnResponseReceived(std::move(response));

  if (!proxy_has_extra_headers() || response_.headers) {
    ContinueToHeadersReceived();
  } else {
    waiting_for_header_client_headers_received_ = true;
  }
}

void BraveProxyingWebSocket::ContinueToHeadersReceived() {
  auto continuation = base::BindRepeating(
      &BraveProxyingWebSocket::OnHeadersReceivedComplete,
      weak_factory_.GetWeakPtr());
  ctx_ = std::make_shared<brave::BraveRequestInfo>();
  brave::BraveRequestInfo::FillCTX(request_, process_id_,
                                   frame_tree_node_id_, request_id_,
                                   browser_context_, ctx_);
  int result = request_handler_->OnHeadersReceived(
      ctx_, continuation, response_.headers.get(),
      &override_headers_, &redirect_url_);

  if (result == net::ERR_BLOCKED_BY_CLIENT ||
      // handle adblock kEmptyDataURI
      redirect_url_ == kEmptyDataURI) {
    OnError(result);
    return;
  }

  PauseIncomingMethodCallProcessing();
  if (result == net::ERR_IO_PENDING)
    return;

  DCHECK_EQ(net::OK, result);
  OnHeadersReceivedComplete(net::OK);
}

void BraveProxyingWebSocket::OnConnectionEstablished(
    mojo::PendingRemote<network::mojom::WebSocket> websocket,
    mojo::PendingReceiver<network::mojom::WebSocketClient> client_receiver,
    const std::string& selected_protocol,
    const std::string& extensions,
    mojo::ScopedDataPipeConsumerHandle readable) {
  DCHECK(forwarding_handshake_client_);
  DCHECK(!is_done_);
  forwarding_handshake_client_->OnConnectionEstablished(
      std::move(websocket), std::move(client_receiver), selected_protocol,
      extensions, std::move(readable));

  OnError(net::ERR_FAILED);
}

void BraveProxyingWebSocket::OnAuthRequired(
    const net::AuthChallengeInfo& auth_info,
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    const net::IPEndPoint& remote_endpoint,
    OnAuthRequiredCallback callback) {
  proxy_auth_handler_->OnAuthRequired(
      auth_info, headers, remote_endpoint, std::move(callback));
}

void BraveProxyingWebSocket::OnBeforeSendHeaders(
    const net::HttpRequestHeaders& headers,
    OnBeforeSendHeadersCallback callback) {
  DCHECK(proxy_has_extra_headers());

  request_.headers = headers;
  on_before_send_headers_callback_ = std::move(callback);
  OnBeforeRequestComplete(net::OK);
}

void BraveProxyingWebSocket::OnHeadersReceived(
    const std::string& headers,
    OnHeadersReceivedCallback callback) {
  DCHECK(proxy_has_extra_headers());

  // Note: since there are different pipes used for WebSocketClient and
  // TrustedHeaderClient, there are no guarantees whether this or
  // OnResponseReceived are called first.
  on_headers_received_callback_ = std::move(callback);
  response_.headers = base::MakeRefCounted<net::HttpResponseHeaders>(headers);

  if (!waiting_for_header_client_headers_received_)
    return;

  waiting_for_header_client_headers_received_ = false;
  ContinueToHeadersReceived();
}

void BraveProxyingWebSocket::OnBeforeRequestComplete(int error_code) {
  DCHECK(proxy_has_extra_headers() || !binding_as_handshake_client_.is_bound());
  DCHECK(request_.url.SchemeIsWSOrWSS());
  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }

  if (proxy_has_extra_headers()) {
    proxy_trusted_header_client_->OnBeforeSendHeaders(
        request_.headers,
        base::BindOnce(
            &BraveProxyingWebSocket::OnBeforeSendHeadersCompleteFromProxy,
            weak_factory_.GetWeakPtr()));
  } else {
    OnBeforeSendHeadersCompleteFromProxy(
        net::OK, request_.headers);
  }
}

void BraveProxyingWebSocket::OnBeforeSendHeadersCompleteFromProxy(
    int error_code,
    const base::Optional<net::HttpRequestHeaders>& headers) {
  DCHECK(proxy_has_extra_headers() || !binding_as_handshake_client_.is_bound());
  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }

  // update the headers from the proxy
  if (headers)
    request_.headers = *headers;
  else
    request_.headers.Clear();

  auto continuation = base::BindRepeating(
      &BraveProxyingWebSocket::OnBeforeSendHeadersComplete,
      weak_factory_.GetWeakPtr());

  ctx_ = std::make_shared<brave::BraveRequestInfo>();
  brave::BraveRequestInfo::FillCTX(request_, process_id_,
                                   frame_tree_node_id_, request_id_,
                                   browser_context_, ctx_);
  int result = request_handler_->OnBeforeStartTransaction(
      ctx_, continuation, &request_.headers);

  if (result == net::ERR_BLOCKED_BY_CLIENT) {
    OnError(result);
    return;
  }

  if (result == net::ERR_IO_PENDING)
    return;

  DCHECK_EQ(net::OK, result);
  continuation.Run(net::OK);
}

void BraveProxyingWebSocket::OnBeforeSendHeadersComplete(int error_code) {
  DCHECK(proxy_has_extra_headers() || !binding_as_handshake_client_.is_bound());

  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }

  if (on_before_send_headers_callback_)
    std::move(on_before_send_headers_callback_).Run(
        error_code, base::Optional<net::HttpRequestHeaders>(request_.headers));

  if (!proxy_has_extra_headers())
    ContinueToStartRequest(error_code);
}

void BraveProxyingWebSocket::ContinueToStartRequest(int error_code) {
  std::vector<network::mojom::HttpHeaderPtr> additional_headers;
  if (!proxy_has_extra_headers()) {
    for (net::HttpRequestHeaders::Iterator it(request_.headers);
         it.GetNext();) {
      additional_headers.push_back(
          network::mojom::HttpHeader::New(it.name(), it.value()));
    }
  }

  network::mojom::WebSocketHandshakeClientPtr handshake_client;
  binding_as_handshake_client_.Bind(mojo::MakeRequest(&handshake_client));
  binding_as_handshake_client_.set_connection_error_with_reason_handler(
      base::BindOnce(&BraveProxyingWebSocket::OnMojoConnectionError,
                     base::Unretained(this)));

  network::mojom::AuthenticationHandlerPtr auth_handler;
  if (proxy_auth_handler_.is_bound())
    binding_as_auth_handler_.Bind(mojo::MakeRequest(&auth_handler));

  network::mojom::TrustedHeaderClientPtr trusted_header_client;
  if (proxy_has_extra_headers())
    binding_as_header_client_.Bind(mojo::MakeRequest(&trusted_header_client));

  std::move(factory_).Run(request_.url,
                          std::move(additional_headers),
                          std::move(handshake_client),
                          std::move(auth_handler),
                          std::move(trusted_header_client));
}

void BraveProxyingWebSocket::OnHeadersReceivedCompleteFromProxy(
    int error_code,
    const base::Optional<std::string>& headers,
    const GURL& url) {
  if (on_headers_received_callback_)
    std::move(on_headers_received_callback_).Run(net::OK, headers, GURL());

  if (override_headers_) {
    response_.headers = override_headers_;
    override_headers_ = nullptr;
  }

  ResumeIncomingMethodCallProcessing();
}

void BraveProxyingWebSocket::OnHeadersReceivedComplete(int error_code) {
  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }

  std::string headers;
  if (override_headers_)
    headers = override_headers_->raw_headers();

  if (proxy_has_extra_headers()) {
    proxy_trusted_header_client_->OnHeadersReceived(
        headers,
        base::BindOnce(
            &BraveProxyingWebSocket::OnHeadersReceivedCompleteFromProxy,
            weak_factory_.GetWeakPtr()));
  } else {
    OnHeadersReceivedCompleteFromProxy(
        error_code, base::Optional<std::string>(headers), GURL());
  }
}

void BraveProxyingWebSocket::PauseIncomingMethodCallProcessing() {
  binding_as_handshake_client_.PauseIncomingMethodCallProcessing();
  if (proxy_has_extra_headers())
    binding_as_header_client_.PauseIncomingMethodCallProcessing();
}

void BraveProxyingWebSocket::ResumeIncomingMethodCallProcessing() {
  binding_as_handshake_client_.ResumeIncomingMethodCallProcessing();
  if (proxy_has_extra_headers())
    binding_as_header_client_.ResumeIncomingMethodCallProcessing();
}

void BraveProxyingWebSocket::OnError(int error_code) {
  if (!is_done_) {
    is_done_ = true;
  }

  // Deletes |this|.
  std::move(on_disconnect_).Run(this);
}

// ResetWithReason
void BraveProxyingWebSocket::OnMojoConnectionError(
    uint32_t custom_reason,
    const std::string& description) {
  forwarding_handshake_client_.ResetWithReason(custom_reason, description);
  OnError(net::ERR_FAILED);
  // Deletes |this|.
}
