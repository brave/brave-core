/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEBUI_CONFIG_MAP_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEBUI_CONFIG_MAP_H_

#include <content/public/browser/webui_config.h>
#include <content/public/browser/webui_config_map.h>  // IWYU pragma: export

#include <string_view>

namespace content {

// Registers `host` (a chrome-untrusted:// host with no subdomain, e.g.
// "leo-workspaces") as a subdomain host. A WebUIConfig registered for `host`
// will then also handle every single-label subdomain of it
// (chrome-untrusted://<sub>.host) as a distinct origin. Call alongside
// WebUIConfigMap::AddUntrustedWebUIConfig().
CONTENT_EXPORT void RegisterUntrustedSubdomainHost(std::string_view host);

}  // namespace content

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_PUBLIC_BROWSER_WEBUI_CONFIG_MAP_H_
