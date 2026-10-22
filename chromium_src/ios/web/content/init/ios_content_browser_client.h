// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_CONTENT_INIT_IOS_CONTENT_BROWSER_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_CONTENT_INIT_IOS_CONTENT_BROWSER_CLIENT_H_

#include "base/functional/callback.h"
#include "content/public/browser/content_browser_client.h"

namespace network {
struct ResourceRequest;
}  // namespace network

#define ConfigureNetworkContextParams                                        \
  BrowserURLHandlerCreated(content::BrowserURLHandler* handler) override;   \
  void WillCreateURLLoaderFactory(                                          \
      content::BrowserContext* browser_context,                             \
      content::RenderFrameHost* frame, int render_process_id,               \
      URLLoaderFactoryType type, const url::Origin& request_initiator,      \
      const net::IsolationInfo& isolation_info,                             \
      std::optional<int64_t> navigation_id,                                 \
      ukm::SourceIdObj ukm_source_id,                                       \
      network::URLLoaderFactoryBuilder& factory_builder,                    \
      mojo::PendingRemote<network::mojom::TrustedURLLoaderHeaderClient>*    \
          header_client,                                                    \
      bool* bypass_redirect_checks, bool* disable_secure_dns,               \
      network::mojom::URLLoaderFactoryOverridePtr* factory_override,        \
      scoped_refptr<base::SequencedTaskRunner>                              \
          navigation_response_task_runner,                                  \
      bool is_for_network_service) override;                                \
  void OnWebContentsCreated(content::WebContents* web_contents) override;   \
  void ConfigureNetworkContextParams

#include <ios/web/content/init/ios_content_browser_client.h>  // IWYU pragma: export

#undef ConfigureNetworkContextParams

namespace web {

// Returns true if a subresource request should fail as blocked by client.
using SubresourceBlocker =
    base::RepeatingCallback<bool(const network::ResourceRequest&)>;

// Must be called on the UI thread.
void SetSubresourceBlocker(SubresourceBlocker blocker);

}  // namespace web

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_CONTENT_INIT_IOS_CONTENT_BROWSER_CLIENT_H_
