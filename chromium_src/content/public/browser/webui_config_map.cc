/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/public/browser/webui_config_map.h"

#include <map>
#include <memory>
#include <string_view>

#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace content {
namespace {

// If `url`'s host is a single-label subdomain of a host with a registered
// chrome-untrusted:// config that opts into handling its own subdomains via
// WebUIConfig::ShouldHandleSubdomains(), returns that config (e.g.
// chrome-untrusted://<id>.leo-workspaces resolves to the config registered for
// chrome-untrusted://leo-workspaces). Otherwise returns `configs.end()`.
// Called from WebUIConfigMap::GetConfig() via the plaster for
// content/public/browser/webui_config_map.cc.
std::map<url::Origin, std::unique_ptr<WebUIConfig>>::iterator
FindConfigForSubdomain(
    std::map<url::Origin, std::unique_ptr<WebUIConfig>>& configs,
    const GURL& url) {
  // Only chrome-untrusted:// WebUIs may be served from subdomains. A subdomain
  // is always a new origin; for chrome:// that new origin would be granted
  // WebUI bindings, which we don't want to hand out implicitly.
  if (url.scheme() != kChromeUIUntrustedScheme) {
    return configs.end();
  }
  const std::string_view host = url.host();
  const size_t dot = host.find('.');
  if (dot == std::string_view::npos) {
    return configs.end();
  }
  GURL::Replacements replacements;
  replacements.SetHostStr(host.substr(dot + 1));
  const auto parent_origin =
      url::Origin::Create(url.ReplaceComponents(replacements));
  const auto origin_and_config = configs.find(parent_origin);
  if (origin_and_config != configs.end() &&
      !origin_and_config->second->ShouldHandleSubdomains()) {
    return configs.end();
  }
  return origin_and_config;
}

}  // namespace
}  // namespace content

#include <content/public/browser/webui_config_map.cc>
