/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/public/browser/webui_config_map.h"

#include <string>
#include <string_view>

#include "base/containers/flat_set.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "content/public/common/url_constants.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace content {
namespace {

// Hosts opted in to serving their single-label subdomains as distinct origins.
base::flat_set<std::string>& GetSubdomainHosts() {
  static base::NoDestructor<base::flat_set<std::string>> hosts;
  return *hosts;
}

// If `url`'s host is a single-label subdomain of a registered subdomain host,
// returns the origin of that parent host (e.g.
// chrome-untrusted://<id>.leo-workspaces -> chrome-untrusted://leo-workspaces).
// Otherwise returns an opaque origin, which never matches a registered config.
// Called inline from WebUIConfigMap::GetConfig() via the plaster for
// content/public/browser/webui_config_map.cc.
url::Origin GetRegisteredParentOrigin(const GURL& url) {
  // The subdomain registry only applies to chrome-untrusted:// hosts.
  if (url.GetScheme() != kChromeUIUntrustedScheme) {
    return url::Origin();
  }
  const std::string host = url.GetHost();
  const size_t dot = host.find('.');
  if (dot == std::string::npos) {
    return url::Origin();
  }
  const std::string parent = host.substr(dot + 1);
  if (!GetSubdomainHosts().contains(parent)) {
    return url::Origin();
  }
  return url::Origin::Create(GURL(
      base::StrCat({url.GetScheme(), url::kStandardSchemeSeparator, parent})));
}

}  // namespace

void RegisterUntrustedSubdomainHost(std::string_view host) {
  GetSubdomainHosts().insert(std::string(host));
}

}  // namespace content

#include <content/public/browser/webui_config_map.cc>

#undef BRAVE_WEBUI_CONFIG_MAP_GET_CONFIG_FALLBACK
