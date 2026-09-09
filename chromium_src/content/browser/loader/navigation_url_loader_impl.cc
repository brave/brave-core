/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/webui/url_data_manager_backend.h"
#include "content/public/browser/web_ui_url_loader_factory.h"
#include "content/public/browser/webui_config.h"
#include "content/public/browser/webui_config_map.h"
#include "url/gurl.h"

namespace content {

class BrowserContext;
class FrameTreeNode;

namespace {

// Whether |url| is a WebUI that has asked to be reachable by a service worker,
// and so must not take the shortcut in NavigationURLLoaderImpl::Start() that
// loads WebUI without consulting any interceptor.
//
// Hosts that have not asked keep the shortcut, so this changes the way a
// navigation is loaded only where a WebUI opted in.
bool ShouldInterceptWebUINavigation(BrowserContext* browser_context,
                                    const GURL& url) {
  WebUIConfig* config =
      WebUIConfigMap::GetInstance().GetConfig(browser_context, url);
  return config && config->ShouldInterceptNavigationsWithServiceWorker();
}

// The factory the shortcut would have used, for an opted-in host whose service
// worker did not serve the navigation after all. Reached through
// CreateTerminalNonNetworkLoaderFactory(), which knows every other non-network
// scheme but not WebUI, because until now WebUI never got that far.
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
