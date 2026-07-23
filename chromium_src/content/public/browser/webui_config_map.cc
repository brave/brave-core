/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/public/browser/webui_config_map.h"

#include <map>
#include <memory>
#include <string>
#include <string_view>

#include "base/strings/strcat.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

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
FindSubdomainConfig(
    std::map<url::Origin, std::unique_ptr<WebUIConfig>>& configs,
    const GURL& url) {
  // Only chrome-untrusted:// WebUIs may be served from subdomains: subdomains
  // of chrome:// hosts would be new origins with WebUI bindings.
  if (url.GetScheme() != kChromeUIUntrustedScheme) {
    return configs.end();
  }
  const std::string host = url.GetHost();
  const size_t dot = host.find('.');
  if (dot == std::string::npos) {
    return configs.end();
  }
  const auto parent_origin = url::Origin::Create(
      GURL(base::StrCat({url.GetScheme(), url::kStandardSchemeSeparator,
                         std::string_view(host).substr(dot + 1)})));
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
