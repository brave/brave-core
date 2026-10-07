/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <string>

#include "content/browser/webui/url_data_manager_backend.h"
#include "content/public/browser/web_ui_url_loader_factory.h"
#include "content/public/browser/webui_config.h"
#include "content/public/browser/webui_config_map.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/mojom/url_loader_factory.mojom.h"
#include "url/gurl.h"

namespace content {

class BrowserContext;
class FrameTreeNode;

namespace {

// Whether |url| is a WebUI that has asked to be reachable by a service worker,
// and so must not take the shortcut in NavigationURLLoaderImpl::Start() that
// loads WebUI without consulting any interceptor.
bool ShouldInterceptWebUINavigation(BrowserContext* browser_context,
                                    const GURL& url) {
  WebUIConfig* config =
      WebUIConfigMap::GetInstance().GetConfig(browser_context, url);
  return config && config->ShouldInterceptNavigationsWithServiceWorker();
}

mojo::PendingRemote<network::mojom::URLLoaderFactory>
MaybeCreateWebUILoaderFactory(FrameTreeNode* frame_tree_node, const GURL& url);

}  // namespace

}  // namespace content

#include <content/browser/loader/navigation_url_loader_impl.cc>

namespace content {
namespace {

mojo::PendingRemote<network::mojom::URLLoaderFactory>
MaybeCreateWebUILoaderFactory(FrameTreeNode* frame_tree_node, const GURL& url) {
  const std::string scheme = url.GetScheme();
  if (!std::ranges::contains(URLDataManagerBackend::GetWebUISchemes(),
                             scheme)) {
    return {};
  }
  return CreateWebUIURLLoaderFactory(frame_tree_node->current_frame_host(),
                                     scheme, /*allowed_hosts=*/{});
}

}  // namespace
}  // namespace content
