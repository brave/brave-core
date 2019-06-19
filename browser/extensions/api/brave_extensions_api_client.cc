/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_extensions_api_client.h"

#include "brave/extensions/common/brave_extension_urls.h"

namespace extensions {

bool BraveExtensionsAPIClient::ShouldHideBrowserNetworkRequest(
    const WebRequestInfo& request) const {
  const GURL& url = request.url;
  if (extension_urls::IsBraveProtectedUrl(url::Origin::Create(url),
                                          url.path_piece())) {
    return true;
  }
  return ChromeExtensionsAPIClient::ShouldHideBrowserNetworkRequest(request);
}

}  // namespace extensions
