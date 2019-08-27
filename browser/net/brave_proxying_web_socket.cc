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
    BraveRequestHandler* handler,
    content::ResourceContext* resource_context,
    int process_id,
    int frame_id,
    int frame_tree_node_id,
    const url::Origin& origin,
    scoped_refptr<RequestIDGenerator> request_id_generator,
    network::mojom::WebSocketPtr proxied_socket,
    network::mojom::WebSocketRequest proxied_request,
    DisconnectCallback on_disconnect)
    : request_handler_(handler),
      process_id_(process_id),
      frame_id_(frame_id),
      frame_tree_node_id_(frame_tree_node_id),
      origin_(origin),
      resource_context_(resource_context),
      request_id_generator_(std::move(request_id_generator)),
      proxied_socket_(std::move(proxied_socket)),
      on_disconnect_(std::move(on_disconnect)) {
  binding_as_websocket_.Bind(std::move(proxied_request));

  binding_as_websocket_.set_connection_error_handler(
      base::BindRepeating(&BraveProxyingWebSocket::OnError,
                          base::Unretained(this), net::ERR_FAILED));
}

BraveProxyingWebSocket::~BraveProxyingWebSocket() {
  if (ctx_) {
    request_handler_->OnURLRequestDestroyed(ctx_);
  }
}

// static
bool BraveProxyingWebSocket::ProxyWebSocket(
    content::RenderFrameHost* frame,
    network::mojom::WebSocketRequest* request,
    network::mojom::AuthenticationHandlerPtr* auth_handler) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  network::mojom::WebSocketPtrInfo proxied_socket_ptr_info;
  auto proxied_request = std::move(*request);
  *request = mojo::MakeRequest(&proxied_socket_ptr_info);

  base::PostTaskWithTraits(
      FROM_HERE, {content::BrowserThread::IO},
      base::BindOnce(&ResourceContextData::StartProxyingWebSocket,
                     frame->GetProcess()->GetBrowserContext()
                         ->GetResourceContext(),
                     frame->GetProcess()->GetID(),
                     frame->GetRoutingID(),
                     frame->GetFrameTreeNodeId(),
                     frame->GetLastCommittedOrigin(),
                     std::move(proxied_socket_ptr_info),
                     std::move(proxied_request)));
  return true;
}

void BraveProxyingWebSocket::AddChannelRequest(
    const GURL& url,
    const std::vector<std::string>& requested_protocols,
    const GURL& site_for_cookies,
    std::vector<network::mojom::HttpHeaderPtr> additional_headers,
    network::mojom::WebSocketClientPtr client) {
  if (binding_as_client_.is_bound() || !client || forwarding_client_) {
    // Illegal request.
    proxied_socket_ = nullptr;
    return;
  }

  request_.url = url;
  // TODO(iefremov): site_for_cookies is not enough, we should find a way
  // to initialize NetworkIsolationKey.
  request_.site_for_cookies = site_for_cookies;
  request_.request_initiator = origin_;
  request_.render_frame_id = frame_id_;
  websocket_protocols_ = requested_protocols;
  request_id_ = request_id_generator_->Generate();

  forwarding_client_ = std::move(client);
  additional_headers_ = std::move(additional_headers);

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

void BraveProxyingWebSocket::SendFrame(
    bool fin,
    network::mojom::WebSocketMessageType type,
    const std::vector<uint8_t>& data) {
  proxied_socket_->SendFrame(fin, type, data);
}

void BraveProxyingWebSocket::AddReceiveFlowControlQuota(int64_t quota) {
  proxied_socket_->AddReceiveFlowControlQuota(quota);
}

void BraveProxyingWebSocket::StartClosingHandshake(
    uint16_t code,
    const std::string& reason) {
  proxied_socket_->StartClosingHandshake(code, reason);
}

void BraveProxyingWebSocket::OnFailChannel(const std::string& reason) {
  DCHECK(forwarding_client_);
  forwarding_client_->OnFailChannel(reason);

  forwarding_client_ = nullptr;
  int rv = net::ERR_FAILED;
  if (reason == "HTTP Authentication failed; no valid credentials available" ||
      reason == "Proxy authentication failed") {
    // This is needed to make some tests pass.
    // TODO(yhirano): Remove this hack.
    rv = net::ERR_ABORTED;
  }

  OnError(rv);
}

void BraveProxyingWebSocket::OnStartOpeningHandshake(
    network::mojom::WebSocketHandshakeRequestPtr request) {
  DCHECK(forwarding_client_);
  forwarding_client_->OnStartOpeningHandshake(std::move(request));
}

void BraveProxyingWebSocket::OnFinishOpeningHandshake(
    network::mojom::WebSocketHandshakeResponsePtr response) {
  DCHECK(forwarding_client_);

  response_.headers =
      base::MakeRefCounted<net::HttpResponseHeaders>(base::StringPrintf(
          "HTTP/%d.%d %d %s", response->http_version.major_value(),
          response->http_version.minor_value(), response->status_code,
          response->status_text.c_str()));
  for (const auto& header : response->headers)
    response_.headers->AddHeader(header->name + ": " + header->value);

  response_.remote_endpoint = response->remote_endpoint;

  // TODO(yhirano): with both network service enabled or disabled,
  // OnFinishOpeningHandshake is called with the original response headers.
  // That means if OnHeadersReceived modified them the renderer won't see that
  // modification. This is the opposite of http(s) requests.
  forwarding_client_->OnFinishOpeningHandshake(std::move(response));

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

void BraveProxyingWebSocket::OnAddChannelResponse(
    const std::string& selected_protocol,
    const std::string& extensions) {
  DCHECK(forwarding_client_);
  DCHECK(!is_done_);
  is_done_ = true;

  forwarding_client_->OnAddChannelResponse(selected_protocol, extensions);
}

void BraveProxyingWebSocket::OnDataFrame(
    bool fin,
    network::mojom::WebSocketMessageType type,
    const std::vector<uint8_t>& data) {
  DCHECK(forwarding_client_);
  forwarding_client_->OnDataFrame(fin, type, data);
}

void BraveProxyingWebSocket::OnFlowControl(int64_t quota) {
  DCHECK(forwarding_client_);
  forwarding_client_->OnFlowControl(quota);
}

void BraveProxyingWebSocket::OnDropChannel(bool was_clean,
                                                uint16_t code,
                                                const std::string& reason) {
  DCHECK(forwarding_client_);
  forwarding_client_->OnDropChannel(was_clean, code, reason);
  forwarding_client_ = nullptr;
  OnError(net::ERR_FAILED);
}

void BraveProxyingWebSocket::OnClosingHandshake() {
  DCHECK(forwarding_client_);
  forwarding_client_->OnClosingHandshake();
}

void BraveProxyingWebSocket::OnBeforeRequestComplete(int error_code) {
  DCHECK(!binding_as_client_.is_bound());
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
  DCHECK(!binding_as_client_.is_bound());
  if (error_code != net::OK) {
    OnError(error_code);
    return;
  }

  network::mojom::WebSocketClientPtr proxy;

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

  binding_as_client_.Bind(mojo::MakeRequest(&proxy));
  binding_as_client_.set_connection_error_handler(
      base::BindOnce(&BraveProxyingWebSocket::OnError,
                     base::Unretained(this), net::ERR_FAILED));
  proxied_socket_->AddChannelRequest(
      request_.url, websocket_protocols_, request_.site_for_cookies,
      std::move(additional_headers), std::move(proxy));
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
  binding_as_client_.PauseIncomingMethodCallProcessing();
}

void BraveProxyingWebSocket::ResumeIncomingMethodCallProcessing() {
  binding_as_client_.ResumeIncomingMethodCallProcessing();
}

void BraveProxyingWebSocket::OnError(int error_code) {
  if (!is_done_) {
    is_done_ = true;
  }
  if (forwarding_client_)
    forwarding_client_->OnFailChannel(net::ErrorToString(error_code));

  // Deletes |this|.
  std::move(on_disconnect_).Run(this);
}
