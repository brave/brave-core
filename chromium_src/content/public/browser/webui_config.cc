/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <content/public/browser/webui_config.cc>

namespace content {

// Whether navigations to this WebUI should go through the
// NavigationURLLoaderImpl interceptors, so that a service worker registered for
// this host can serve them.
//
// WebUI navigations otherwise take a shortcut that returns before any
// interceptor is consulted, which leaves such a worker unreachable - for a
// document's subresources as much as for the navigation itself, since the
// document never gets a service worker client either. Only a host that serves
// its own content from a worker has any reason to leave that shortcut.
bool WebUIConfig::ShouldInterceptNavigationsWithServiceWorker() {
  return false;
}

}  // namespace content
