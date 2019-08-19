/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_proxying_utils.h"

#include <utility>

#include "brave/browser/net/brave_proxying_url_loader_factory.h"
#include "brave/browser/net/brave_proxying_web_socket.h"
#include "brave/browser/net/brave_request_handler.h"
#include "content/public/browser/resource_context.h"

// User data key for ResourceContextData.
const void* const kResourceContextUserDataKey = &kResourceContextUserDataKey;

ResourceContextData::ResourceContextData()
    : request_id_generator_(base::MakeRefCounted<RequestIDGenerator>()),
      weak_factory_(this) {}

ResourceContextData::~ResourceContextData() = default;

// static
void ResourceContextData::StartProxying(
    content::ResourceContext* resource_context,
    int render_process_id,
    int frame_tree_node_id,
    network::mojom::URLLoaderFactoryRequest request,
    network::mojom::URLLoaderFactoryPtrInfo target_factory) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  auto* self = static_cast<ResourceContextData*>(
      resource_context->GetUserData(kResourceContextUserDataKey));
  if (!self) {
    self = new ResourceContextData();
    resource_context->SetUserData(kResourceContextUserDataKey,
                                  base::WrapUnique(self));
  }

  if (!self->request_handler_) {
    self->request_handler_.reset(new BraveRequestHandler);
  }

  auto proxy = std::make_unique<BraveProxyingURLLoaderFactory>(
      self->request_handler_.get(), resource_context, render_process_id,
      frame_tree_node_id, std::move(request), std::move(target_factory),
      self->request_id_generator_,
      base::BindOnce(&ResourceContextData::RemoveProxy,
                     self->weak_factory_.GetWeakPtr()));

  self->proxies_.emplace(std::move(proxy));
}

// static
void ResourceContextData::StartProxyingWebSocket(
    content::ResourceContext* resource_context,
    int render_process_id,
    int frame_id,
    int frame_tree_node_id,
    const url::Origin& origin,
    network::mojom::WebSocketPtrInfo proxied_socket_ptr_info,
    network::mojom::WebSocketRequest proxied_request) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  auto* self = static_cast<ResourceContextData*>(
      resource_context->GetUserData(kResourceContextUserDataKey));
  if (!self) {
    self = new ResourceContextData();
    resource_context->SetUserData(kResourceContextUserDataKey,
                                  base::WrapUnique(self));
  }

  if (!self->request_handler_) {
    self->request_handler_.reset(new BraveRequestHandler);
  }

  auto proxy = std::make_unique<BraveProxyingWebSocket>(
      self->request_handler_.get(), resource_context, render_process_id,
      frame_id, frame_tree_node_id, origin,
      self->request_id_generator_,
      network::mojom::WebSocketPtr(std::move(proxied_socket_ptr_info)),
      std::move(proxied_request),
      base::BindOnce(&ResourceContextData::RemoveProxyWebSocket,
                     self->weak_factory_.GetWeakPtr()));

  self->websocket_proxies_.emplace(std::move(proxy));
}


void ResourceContextData::RemoveProxy(BraveProxyingURLLoaderFactory* proxy) {
  auto it = proxies_.find(proxy);
  DCHECK(it != proxies_.end());
  proxies_.erase(it);
}

void ResourceContextData::RemoveProxyWebSocket(BraveProxyingWebSocket* proxy) {
  auto it = websocket_proxies_.find(proxy);
  DCHECK(it != websocket_proxies_.end());
  websocket_proxies_.erase(it);
}

