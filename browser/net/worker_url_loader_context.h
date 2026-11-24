/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_WORKER_URL_LOADER_CONTEXT_H_
#define BRAVE_BROWSER_NET_WORKER_URL_LOADER_CONTEXT_H_

#include "base/notreached.h"
#include "content/public/browser/content_browser_client.h"

namespace brave {

// Worker URLLoader factories cover worker script loads and fetch/XHR-style
// requests made by workers. Worker WebSocket handshakes are handled by
// BraveProxyingWebSocket instead. These URLLoader factories can be created
// without a RenderFrameHost, and requests made through them do not always carry
// TrustedParams. Use the context captured when those factories were created as
// a fallback. Other factory types already get their context from the request or
// frame; retaining that path avoids changing their behavior when a factory is
// reused or a request redirects.
inline bool ShouldUseFactoryURLLoaderContext(
    content::ContentBrowserClient::URLLoaderFactoryType type) {
  using URLLoaderFactoryType =
      content::ContentBrowserClient::URLLoaderFactoryType;
  switch (type) {
    case URLLoaderFactoryType::kWorkerMainResource:
    case URLLoaderFactoryType::kWorkerSubResource:
    case URLLoaderFactoryType::kServiceWorkerScript:
    case URLLoaderFactoryType::kServiceWorkerSubResource:
      return true;
    case URLLoaderFactoryType::kNavigation:
    case URLLoaderFactoryType::kDownload:
    case URLLoaderFactoryType::kDocumentSubResource:
    case URLLoaderFactoryType::kPrefetch:
    case URLLoaderFactoryType::kDevTools:
    case URLLoaderFactoryType::kEarlyHints:
      return false;
  }
  NOTREACHED();
}

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_WORKER_URL_LOADER_CONTEXT_H_
