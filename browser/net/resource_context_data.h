/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_RESOURCE_CONTEXT_DATA_H_
#define BRAVE_BROWSER_NET_RESOURCE_CONTEXT_DATA_H_

#include <cstdint>
#include <memory>
#include <set>
#include <string>

#include "base/containers/unique_ptr_adapters.h"
#include "base/memory/ref_counted.h"
#include "base/supports_user_data.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/content_browser_client.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "services/network/public/mojom/websocket.mojom.h"

class BraveProxyingURLLoaderFactory;
class BraveProxyingWebSocket;
class BraveRequestHandler;

namespace content {
class BrowserContext;
}

// Used for both URLLoaders and WebSocket proxies.
class RequestIDGenerator
    : public base::RefCountedThreadSafe<RequestIDGenerator> {
 public:
  RequestIDGenerator() = default;
  RequestIDGenerator(const RequestIDGenerator&) = delete;
  RequestIDGenerator& operator=(const RequestIDGenerator&) = delete;
  int64_t Generate() {
    DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
    return ++id_;
  }

 private:
  friend class base::RefCountedThreadSafe<RequestIDGenerator>;
  ~RequestIDGenerator() {}

  // Although this initialization can be done in a thread other than the IO
  // thread, we expect at least one memory barrier before actually calling
  // Generate in the IO thread, so we don't protect the variable with a lock.
  int64_t id_ = 0;
};

// Owns proxying factories for URLLoaders and websocket proxies. There is
// one |ResourceContextData| per profile.
class ResourceContextData : public base::SupportsUserData::Data {
 public:
  ResourceContextData(const ResourceContextData&) = delete;
  ResourceContextData& operator=(const ResourceContextData&) = delete;
  ~ResourceContextData() override;

  static void StartProxying(
      content::BrowserContext* browser_context,
      int render_process_id,
      int frame_tree_node_id,
      network::mojom::URLLoaderFactoryRequest request,
      network::mojom::URLLoaderFactoryPtrInfo target_factory);

  static BraveProxyingWebSocket* StartProxyingWebSocket(
      content::ContentBrowserClient::WebSocketFactory factory,
      const GURL& url,
      const net::SiteForCookies& site_for_cookies,
      const absl::optional<std::string>& user_agent,
      mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
          handshake_client,
      content::BrowserContext* browser_context,
      int render_process_id,
      int frame_id,
      int frame_tree_node_id,
      const url::Origin& origin);

  void RemoveProxy(BraveProxyingURLLoaderFactory* proxy);
  void RemoveProxyWebSocket(BraveProxyingWebSocket* proxy);

 private:
  ResourceContextData();

  std::unique_ptr<BraveRequestHandler> request_handler_;
  scoped_refptr<RequestIDGenerator> request_id_generator_;

  std::set<std::unique_ptr<BraveProxyingURLLoaderFactory>,
           base::UniquePtrComparator>
      proxies_;

  std::set<std::unique_ptr<BraveProxyingWebSocket>,
           base::UniquePtrComparator>
      websocket_proxies_;

  base::WeakPtrFactory<ResourceContextData> weak_factory_;
};

#endif  // BRAVE_BROWSER_NET_RESOURCE_CONTEXT_DATA_H_
