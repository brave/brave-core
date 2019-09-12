/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_extensions_api_client.h"

#include "extensions/common/permissions/permissions_data.h"

namespace extensions {

bool IsBraveSecurityUrl(const GURL& url) {
  return url.DomainIs("sb-ssl.brave.com") ||
      url.DomainIs("safebrowsing.brave.com");
}

bool BraveExtensionsAPIClient::ShouldHideBrowserNetworkRequest(
    content::BrowserContext* context,
    const WebRequestInfo& request) const {
  if (IsBraveProtectedUrl(request.url) || IsBraveSecurityUrl(request.url)) {
    return true;
  }
  return ChromeExtensionsAPIClient::ShouldHideBrowserNetworkRequest(context,
                                                                    request);
}

}  // namespace extensions
