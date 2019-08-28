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
    content::ResourceContext* resource_context,
    scoped_refptr<RequestIDGenerator> request_id_generator,
    BraveRequestHandler* handler,
    DisconnectCallback on_disconnect)
    : request_handler_(handler),
      process_id_(process_id),
      frame_tree_node_id_(frame_tree_node_id),
      factory_(std::move(factory)),
      resource_context_(resource_context),
      request_id_generator_(std::move(request_id_generator)),
      forwarding_handshake_client_(std::move(handshake_client)),
      request_(request),
      on_disconnect_(std::move(on_disconnect)) {}

BraveProxyingWebSocket::~BraveProxyingWebSocket() {
  if (ctx_) {
    request_handler_->OnURLRequestDestroyed(ctx_);
  }
}

// static
bool BraveProxyingWebSocket::ProxyWebSocket(
    content::RenderFrameHost* frame,
    content::ContentBrowserClient::WebSocketFactory factory,
    const GURL& url,
    const GURL& site_for_cookies,
    const base::Optional<std::string>& user_agent,
    network::mojom::WebSocketHandshakeClientPtr handshake_client) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::IO},
      base::BindOnce(
          &ResourceContextData::StartProxyingWebSocket, std::move(factory), url,
          site_for_cookies, user_agent, handshake_client.PassInterface(),
          frame->GetProcess()->GetBrowserContext()->GetResourceContext(),
          frame->GetProcess()->GetID(), frame->GetRoutingID(),
          frame->GetFrameTreeNodeId(), frame->GetLastCommittedOrigin()));
  return true;
}

void BraveProxyingWebSocket::Start() {
  request_id_ = request_id_generator_->Generate();
  // If the header client will be used, we start the request immediately, and
  // OnBeforeSendHeaders and OnSendHeaders will be handled there. Otherwise,
  // send these events before the request starts.
  base::RepeatingCallback<void(int)> continuation;
  continuation = base::BindRepeating(
      &BraveProxyingWebSocket::OnBeforeRequestComplete,
      weak_factory_.GetWeakPtr());

  // TODO(yhirano): Consider having throttling here (probably with aligned with
  // WebRequestProxyingURLLoaderFactory).
  bool should_collapse_initiator = false;
  ctx_ = std::make_shared<brave::BraveRequestInfo>();
  brave::BraveRequestInfo::FillCTX(request_, process_id_,
                                   frame_tree_node_id_, request_id_,
                                   resource_context_, ctx_);
  int result = request_handler_->OnBeforeURLRequest(
      ctx_, continuation, &redirect_url_);

  // It doesn't make sense to collapse WebSocket requests since they won't be
  // associated with a DOM element.
  DCHECK(!should_collapse_initiator);

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

void BraveProxyingWebSocket::OnOpeningHandshakeStarted(
    network::mojom::WebSocketHandshakeRequestPtr request) {
  DCHECK(forwarding_handshake_client_);
  forwarding_handshake_client_->OnOpeningHandshakeStarted(std::move(request));
}

void BraveProxyingWebSocket::OnResponseReceived(
    network::mojom::WebSocketHandshakeResponsePtr response) {
  DCHECK(forwarding_handshake_client_);

  response_.headers =
      base::MakeRefCounted<net::HttpResponseHeaders>(base::StringPrintf(
          "HTTP/%d.%d %d %s", response->http_version.major_value(),
          response->http_version.minor_value(), response->status_code,
          response->status_text.c_str()));
  for (const auto& header : response->headers)
    response_.headers->AddHeader(header->name + ": " + header->value);

  response_.remote_endpoint = response->remote_endpoint;

  // TODO(yhirano): with both network service enabled or disabled,
  // OnResponseReceived is called with the original response headers.
  // That means if OnHeadersReceived modified them the renderer won't see that
  // modification. This is the opposite of http(s) requests.
  forwarding_handshake_client_->OnResponseReceived(std::move(response));

  ContinueToHeadersReceived();
}

void BraveProxyingWebSocket::ContinueToHeadersReceived() {
  auto continuation = base::BindRepeating(
      &BraveProxyingWebSocket::OnHeadersReceivedComplete,
      weak_factory_.GetWeakPtr());
  ctx_ = std::make_shared<brave::BraveRequestInfo>();
  brave::BraveRequestInfo::FillCTX(request_, process_id_,
                                   frame_tree_node_id_, request_id_,
                                   resource_context_, ctx_);
  int result = request_handler_->OnHeadersReceived(
      ctx_, continuation, response_.headers.get(),
      &override_headers_, &redirect_url_);

  if (result == net::ERR_BLOCKED_BY_CLIENT) {
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
    network::mojom::WebSocketPtr websocket,
    const std::string& selected_protocol,
    const std::string& extensions,
    uint64_t receive_quota_threshold) {
  DCHECK(forwarding_handshake_client_);
  DCHECK(!is_done_);
  is_done_ = true;

  forwarding_handshake_client_->OnConnectionEstablished(
      std::move(websocket), selected_protocol, extensions,
      receive_quota_threshold);

  // Deletes |this|.
  std::move(on_disconnect_).Run(this);
}

void BraveProxyingWebSocket::OnBeforeRequestComplete(int error_code) {
  DCHECK(!binding_as_handshake_client_.is_bound());
  DCHECK(request_.url.SchemeIsWSOrWSS());
  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }

  auto continuation = base::BindRepeating(
      &BraveProxyingWebSocket::ContinueToStartRequest,
      weak_factory_.GetWeakPtr());

  ctx_ = std::make_shared<brave::BraveRequestInfo>();
  brave::BraveRequestInfo::FillCTX(request_, process_id_,
                                   frame_tree_node_id_, request_id_,
                                   resource_context_, ctx_);
  int result = request_handler_->OnBeforeStartTransaction(
      ctx_, continuation, &request_.headers);

  if (result == net::ERR_BLOCKED_BY_CLIENT) {
    OnError(result);
    return;
  }

  if (result == net::ERR_IO_PENDING)
    return;

  DCHECK_EQ(net::OK, result);
  ContinueToStartRequest(net::OK);
}

void BraveProxyingWebSocket::ContinueToStartRequest(int error_code) {
  DCHECK(!binding_as_handshake_client_.is_bound());
  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }

  base::flat_set<std::string> used_header_names;
  std::vector<network::mojom::HttpHeaderPtr> additional_headers;
  for (net::HttpRequestHeaders::Iterator it(request_.headers); it.GetNext();) {
    additional_headers.push_back(
        network::mojom::HttpHeader::New(it.name(), it.value()));
    used_header_names.insert(base::ToLowerASCII(it.name()));
  }
  for (const auto& header : additional_headers_) {
    if (!used_header_names.contains(base::ToLowerASCII(header->name))) {
      additional_headers.push_back(
          network::mojom::HttpHeader::New(header->name, header->value));
    }
  }

  // Here we detect mojo connection errors on |handshake_client|. See also
  // CreateWebSocket in //network/services/public/mojom/network_context.mojom.
  // Here we don't have |connection_client| so using |handshake_client| is the
  // best.
  network::mojom::WebSocketHandshakeClientPtr handshake_client;
  binding_as_handshake_client_.Bind(mojo::MakeRequest(&handshake_client));
  binding_as_handshake_client_.set_connection_error_handler(
      base::BindOnce(&BraveProxyingWebSocket::OnError, base::Unretained(this),
                     net::ERR_FAILED));
  network::mojom::AuthenticationHandlerPtr auth_handler;
  network::mojom::TrustedHeaderClientPtr trusted_header_client;

  std::move(factory_).Run(request_.url, std::move(additional_headers),
                          std::move(handshake_client), std::move(auth_handler),
                          std::move(trusted_header_client));
}

void BraveProxyingWebSocket::OnHeadersReceivedComplete(int error_code) {
  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }

  if (override_headers_) {
    response_.headers = override_headers_;
    override_headers_ = nullptr;
  }

  ResumeIncomingMethodCallProcessing();
}

void BraveProxyingWebSocket::PauseIncomingMethodCallProcessing() {
  binding_as_handshake_client_.PauseIncomingMethodCallProcessing();
}

void BraveProxyingWebSocket::ResumeIncomingMethodCallProcessing() {
  binding_as_handshake_client_.ResumeIncomingMethodCallProcessing();
}

void BraveProxyingWebSocket::OnError(int error_code) {
  if (!is_done_) {
    is_done_ = true;
  }

  // Deletes |this|.
  std::move(on_disconnect_).Run(this);
}
