/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_proxying_web_socket.h"

#include <optional>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "brave/browser/net/brave_request_handler.h"
#include "brave/browser/net/url_context.h"
#include "brave/components/constants/network_constants.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "net/cookies/site_for_cookies.h"

template <template <typename> class T>
BraveProxyingWebSocket<T>::BraveProxyingWebSocket(
    WebSocketFactory factory,
    const network::ResourceRequest& request,
    content::GlobalRenderFrameHostToken render_frame_token,
    content::BrowserContext* browser_context,
    scoped_refptr<RequestIDGenerator> request_id_generator,
    BraveRequestHandler<T>& handler,
    DisconnectCallback on_disconnect)
    : request_handler_(handler),
      render_frame_token_(render_frame_token),
      factory_(std::move(factory)),
      browser_context_(browser_context),
      request_id_generator_(std::move(request_id_generator)),
      receiver_as_handshake_client_(this),
      receiver_as_auth_handler_(this),
      receiver_as_header_client_(this),
      request_(request),
      on_disconnect_(std::move(on_disconnect)) {}

template <template <typename> class T>
BraveProxyingWebSocket<T>::~BraveProxyingWebSocket() {
  if (ctx_) {
    request_handler_->OnURLRequestDestroyed(ctx_);
  }
  if (on_before_send_headers_callback_) {
    std::move(on_before_send_headers_callback_)
        .Run(net::ERR_ABORTED, std::nullopt);
  }
  if (on_headers_received_callback_) {
    std::move(on_headers_received_callback_)
        .Run(net::ERR_ABORTED, std::nullopt, std::nullopt);
  }
}

// static
template <template <typename> class T>
BraveProxyingWebSocket<T>* BraveProxyingWebSocket<T>::ProxyWebSocket(
    content::RenderFrameHost* frame,
    content::ContentBrowserClient::WebSocketFactory factory,
    const GURL& url,
    const net::SiteForCookies& site_for_cookies,
    const std::optional<std::string>& user_agent) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  return ResourceContextData<T>::CreateProxyingWebSocket(
      std::move(factory), url, site_for_cookies, user_agent,
      frame->GetProcess()->GetBrowserContext(), frame->GetGlobalFrameToken(),
      frame->GetLastCommittedOrigin());
}

template <>
void BraveProxyingWebSocket<std::shared_ptr>::CreateBraveRequestInfo() {
  CHECK(!ctx_owned_);
  ctx_ = brave::BraveRequestInfo::MakeCTX(
      request_, render_frame_token_, request_id_, browser_context_, ctx_.get());
}

template <>
void BraveProxyingWebSocket<base::WeakPtr>::CreateBraveRequestInfo() {
  if (ctx_) {
    request_handler_->OnURLRequestDestroyed(ctx_);
  }
  ctx_owned_ = brave::BraveRequestInfo::MakeCTX(request_, render_frame_token_,
                                                request_id_, browser_context_,
                                                ctx_owned_.get());
  ctx_ = ctx_owned_->AsWeakPtr();
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::Start(
    mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
        handshake_client) {
  forwarding_handshake_client_.Bind(std::move(handshake_client));
  forwarding_handshake_client_.set_disconnect_with_reason_handler(
      base::BindOnce(&BraveProxyingWebSocket::OnMojoConnectionError,
                     weak_factory_.GetWeakPtr()));

  request_id_ = request_id_generator_->Generate();
  // If the header client will be used, we start the request immediately, and
  // OnBeforeSendHeaders and OnSendHeaders will be handled there. Otherwise,
  // send these events before the request starts.
  base::RepeatingCallback<void(int)> continuation;
  if (proxy_has_extra_headers()) {
    continuation =
        base::BindRepeating(&BraveProxyingWebSocket::ContinueToStartRequest,
                            weak_factory_.GetWeakPtr());
  } else {
    continuation =
        base::BindRepeating(&BraveProxyingWebSocket::OnBeforeRequestComplete,
                            weak_factory_.GetWeakPtr());
  }

  CreateBraveRequestInfo();

  int result =
      request_handler_->OnBeforeURLRequest(ctx_, continuation, &redirect_url_);
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

template <template <typename> class T>
content::ContentBrowserClient::WebSocketFactory
BraveProxyingWebSocket<T>::CreateWebSocketFactory() {
  return base::BindOnce(&BraveProxyingWebSocket::WebSocketFactoryRun,
                        weak_factory_.GetWeakPtr());
}

template <template <typename> class T>
bool BraveProxyingWebSocket<T>::proxy_has_extra_headers() {
  return proxy_trusted_header_client_.is_bound();
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::WebSocketFactoryRun(
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
  proxy_auth_handler_.Bind(std::move(auth_handler));

  if (trusted_header_client) {
    proxy_trusted_header_client_.Bind(std::move(trusted_header_client));
  }

  if (!proxy_has_extra_headers()) {
    for (const auto& header : additional_headers) {
      request_.headers.SetHeader(header->name, header->value);
    }
  }

  Start(std::move(handshake_client));
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::OnOpeningHandshakeStarted(
    network::mojom::WebSocketHandshakeRequestPtr request) {
  DCHECK(forwarding_handshake_client_);
  forwarding_handshake_client_->OnOpeningHandshakeStarted(std::move(request));
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::ContinueToHeadersReceived() {
  auto continuation =
      base::BindRepeating(&BraveProxyingWebSocket::OnHeadersReceivedComplete,
                          weak_factory_.GetWeakPtr());
  CreateBraveRequestInfo();

  int result = request_handler_->OnHeadersReceived(
      ctx_, continuation, response_.headers.get(), &override_headers_,
      &redirect_url_);

  if (result == net::ERR_BLOCKED_BY_CLIENT ||
      // handle adblock kEmptyDataURI
      redirect_url_ == kEmptyDataURI) {
    OnError(result);
    return;
  }

  PauseIncomingMethodCallProcessing();
  if (result == net::ERR_IO_PENDING) {
    return;
  }

  DCHECK_EQ(net::OK, result);
  OnHeadersReceivedComplete(net::OK);
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::OnFailure(const std::string& message,
                                          int32_t net_error,
                                          int32_t response_code) {}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::OnConnectionEstablished(
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

template <template <typename> class T>
void BraveProxyingWebSocket<T>::OnAuthRequired(
    const net::AuthChallengeInfo& auth_info,
    const scoped_refptr<net::HttpResponseHeaders>& headers,
    const net::IPEndPoint& remote_endpoint,
    OnAuthRequiredCallback callback) {
  proxy_auth_handler_->OnAuthRequired(auth_info, headers, remote_endpoint,
                                      std::move(callback));
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::OnBeforeSendHeaders(
    const net::HttpRequestHeaders& headers,
    OnBeforeSendHeadersCallback callback) {
  DCHECK(proxy_has_extra_headers());

  request_.headers = headers;
  on_before_send_headers_callback_ = std::move(callback);
  OnBeforeRequestComplete(net::OK);
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::OnHeadersReceived(
    const std::string& headers,
    const ::net::IPEndPoint& remote_endpoint,
    const std::optional<net::SSLInfo>& ssl_info,
    OnHeadersReceivedCallback callback) {
  DCHECK(proxy_has_extra_headers());

  on_headers_received_callback_ = std::move(callback);
  response_.headers = base::MakeRefCounted<net::HttpResponseHeaders>(headers);

  ContinueToHeadersReceived();
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::OnBeforeRequestComplete(int error_code) {
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
    OnBeforeSendHeadersCompleteFromProxy(net::OK, request_.headers);
  }
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::OnBeforeSendHeadersCompleteFromProxy(
    int error_code,
    const std::optional<net::HttpRequestHeaders>& headers) {
  DCHECK(proxy_has_extra_headers() ||
         !receiver_as_handshake_client_.is_bound());
  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }

  // update the headers from the proxy
  if (headers) {
    request_.headers = *headers;
  } else {
    request_.headers.Clear();
  }

  auto continuation =
      base::BindRepeating(&BraveProxyingWebSocket::OnBeforeSendHeadersComplete,
                          weak_factory_.GetWeakPtr());

  CreateBraveRequestInfo();
  int result = request_handler_->OnBeforeStartTransaction(ctx_, continuation,
                                                          &request_.headers);

  if (result == net::ERR_BLOCKED_BY_CLIENT) {
    OnError(result);
    return;
  }

  if (result == net::ERR_IO_PENDING) {
    return;
  }

  DCHECK_EQ(net::OK, result);
  continuation.Run(net::OK);
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::OnBeforeSendHeadersComplete(int error_code) {
  DCHECK(proxy_has_extra_headers() ||
         !receiver_as_handshake_client_.is_bound());

  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }

  if (on_before_send_headers_callback_) {
    std::move(on_before_send_headers_callback_)
        .Run(error_code,
             std::optional<net::HttpRequestHeaders>(request_.headers));
  }

  if (!proxy_has_extra_headers()) {
    ContinueToStartRequest(error_code);
  }
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::ContinueToStartRequest(int error_code) {
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
  if (proxy_has_extra_headers()) {
    trusted_header_client =
        receiver_as_header_client_.BindNewPipeAndPassRemote();
  }

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
                     weak_factory_.GetWeakPtr()));
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::OnHeadersReceivedCompleteFromProxy(
    int error_code,
    const std::optional<std::string>& headers,
    const std::optional<GURL>& url) {
  if (on_headers_received_callback_) {
    std::move(on_headers_received_callback_)
        .Run(net::OK, headers, std::nullopt);
  }

  if (override_headers_) {
    response_.headers = override_headers_;
    override_headers_ = nullptr;
  }

  ResumeIncomingMethodCallProcessing();
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::OnHeadersReceivedComplete(int error_code) {
  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }

  std::string headers;
  if (override_headers_) {
    headers = override_headers_->raw_headers();
  }

  if (proxy_has_extra_headers()) {
    proxy_trusted_header_client_->OnHeadersReceived(
        headers, remote_endpoint_, /*ssl_info=*/std::nullopt,
        base::BindOnce(
            &BraveProxyingWebSocket::OnHeadersReceivedCompleteFromProxy,
            weak_factory_.GetWeakPtr()));
  } else {
    OnHeadersReceivedCompleteFromProxy(
        error_code, std::optional<std::string>(headers), std::nullopt);
  }
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::PauseIncomingMethodCallProcessing() {
  receiver_as_handshake_client_.Pause();
  if (proxy_has_extra_headers()) {
    receiver_as_header_client_.Pause();
  }
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::ResumeIncomingMethodCallProcessing() {
  receiver_as_handshake_client_.Resume();
  if (proxy_has_extra_headers()) {
    receiver_as_header_client_.Resume();
  }
}

template <template <typename> class T>
void BraveProxyingWebSocket<T>::OnError(int error_code) {
  if (!is_done_) {
    is_done_ = true;
  }

  // Deletes |this|.
  std::move(on_disconnect_).Run(this);
}

// ResetWithReason
template <template <typename> class T>
void BraveProxyingWebSocket<T>::OnMojoConnectionError(
    uint32_t custom_reason,
    const std::string& description) {
  if (forwarding_handshake_client_.is_bound()) {
    forwarding_handshake_client_.ResetWithReason(custom_reason, description);
  }
  OnError(net::ERR_FAILED);
  // Deletes |this|.
}

template class BraveProxyingWebSocket<std::shared_ptr>;
template class BraveProxyingWebSocket<base::WeakPtr>;
