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
#include "net/cookies/site_for_cookies.h"

BraveProxyingWebSocket::BraveProxyingWebSocket(
    WebSocketFactory factory,
    const network::ResourceRequest& request,
    mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
        handshake_client,
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
      receiver_as_handshake_client_(this),
      receiver_as_auth_handler_(this),
      receiver_as_header_client_(this),
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
    const net::SiteForCookies& site_for_cookies,
    const base::Optional<std::string>& user_agent,
    mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
        handshake_client) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  return ResourceContextData::StartProxyingWebSocket(
      std::move(factory), url, site_for_cookies, user_agent,
      std::move(handshake_client), frame->GetProcess()->GetBrowserContext(),
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

  ctx_ = brave::BraveRequestInfo::MakeCTX(request_, process_id_,
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

mojo::Remote<network::mojom::WebSocketHandshakeClient>
BraveProxyingWebSocket::handshake_client() {
  return std::move(forwarding_handshake_client_);
}

bool BraveProxyingWebSocket::proxy_has_extra_headers() {
  return proxy_trusted_header_client_.is_bound();
}

void BraveProxyingWebSocket::WebSocketFactoryRun(
    const GURL& url,
    std::vector<network::mojom::HttpHeaderPtr> additional_headers,
    mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
        handshake_client,
    mojo::PendingRemote<network::mojom::WebSocketAuthenticationHandler>
        auth_handler,
    mojo::PendingRemote<network::mojom::TrustedHeaderClient>
        trusted_header_client) {
  DCHECK(!forwarding_handshake_client_);
  proxy_url_ = url;
  forwarding_handshake_client_.Bind(std::move(handshake_client));
  proxy_auth_handler_.Bind(std::move(auth_handler));

  if (trusted_header_client)
    proxy_trusted_header_client_.Bind(std::move(trusted_header_client));

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

void BraveProxyingWebSocket::ContinueToHeadersReceived() {
  auto continuation = base::BindRepeating(
      &BraveProxyingWebSocket::OnHeadersReceivedComplete,
      weak_factory_.GetWeakPtr());
  ctx_ = brave::BraveRequestInfo::MakeCTX(request_, process_id_,
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

void BraveProxyingWebSocket::OnFailure(const std::string& message,
                                       int32_t net_error,
                                       int32_t response_code) {}

void BraveProxyingWebSocket::OnConnectionEstablished(
    mojo::PendingRemote<network::mojom::WebSocket> websocket,
    mojo::PendingReceiver<network::mojom::WebSocketClient> client_receiver,
    network::mojom::WebSocketHandshakeResponsePtr response,
    mojo::ScopedDataPipeConsumerHandle readable,
    mojo::ScopedDataPipeProducerHandle writable) {
  DCHECK(forwarding_handshake_client_);
  DCHECK(!is_done_);
  remote_endpoint_ = response->remote_endpoint;
  forwarding_handshake_client_->OnConnectionEstablished(
      std::move(websocket), std::move(client_receiver), std::move(response),
      std::move(readable), std::move(writable));

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
    const ::net::IPEndPoint& remote_endpoint,
    OnHeadersReceivedCallback callback) {
  DCHECK(proxy_has_extra_headers());

  on_headers_received_callback_ = std::move(callback);
  response_.headers = base::MakeRefCounted<net::HttpResponseHeaders>(headers);

  ContinueToHeadersReceived();
}

void BraveProxyingWebSocket::OnBeforeRequestComplete(int error_code) {
  DCHECK(proxy_has_extra_headers() ||
         !receiver_as_handshake_client_.is_bound());
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
  DCHECK(proxy_has_extra_headers() ||
         !receiver_as_handshake_client_.is_bound());
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

  ctx_ = brave::BraveRequestInfo::MakeCTX(request_, process_id_,
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
  DCHECK(proxy_has_extra_headers() ||
         !receiver_as_handshake_client_.is_bound());

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
  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }

  std::vector<network::mojom::HttpHeaderPtr> additional_headers;
  if (!proxy_has_extra_headers()) {
    for (net::HttpRequestHeaders::Iterator it(request_.headers);
         it.GetNext();) {
      additional_headers.push_back(
          network::mojom::HttpHeader::New(it.name(), it.value()));
    }
  }

  mojo::PendingRemote<network::mojom::TrustedHeaderClient>
      trusted_header_client = mojo::NullRemote();
  if (proxy_has_extra_headers())
    trusted_header_client =
        receiver_as_header_client_.BindNewPipeAndPassRemote();

  std::move(factory_).Run(
      request_.url, std::move(additional_headers),
      receiver_as_handshake_client_.BindNewPipeAndPassRemote(),
      receiver_as_auth_handler_.BindNewPipeAndPassRemote(),
      std::move(trusted_header_client));

  // Here we detect mojo connection errors on |receiver_as_handshake_client_|.
  // See also CreateWebSocket in
  // //network/services/public/mojom/network_context.mojom.
  receiver_as_handshake_client_.set_disconnect_with_reason_handler(
      base::BindOnce(&BraveProxyingWebSocket::OnMojoConnectionError,
                     base::Unretained(this)));
}

void BraveProxyingWebSocket::OnHeadersReceivedCompleteFromProxy(
    int error_code,
    const base::Optional<std::string>& headers,
    const base::Optional<GURL>& url) {
  if (on_headers_received_callback_)
    std::move(on_headers_received_callback_)
        .Run(net::OK, headers, base::nullopt);

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
        remote_endpoint_,
        base::BindOnce(
            &BraveProxyingWebSocket::OnHeadersReceivedCompleteFromProxy,
            weak_factory_.GetWeakPtr()));
  } else {
    OnHeadersReceivedCompleteFromProxy(
        error_code, base::Optional<std::string>(headers), base::nullopt);
  }
}

void BraveProxyingWebSocket::PauseIncomingMethodCallProcessing() {
  receiver_as_handshake_client_.Pause();
  if (proxy_has_extra_headers())
    receiver_as_header_client_.Pause();
}

void BraveProxyingWebSocket::ResumeIncomingMethodCallProcessing() {
  receiver_as_handshake_client_.Resume();
  if (proxy_has_extra_headers())
    receiver_as_header_client_.Resume();
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
