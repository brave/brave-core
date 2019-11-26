/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_extensions_api_client.h"

#include "base/strings/string_piece.h"
#include "extensions/common/permissions/permissions_data.h"
#include "url/origin.h"

namespace extensions {

bool BraveExtensionsAPIClient::ShouldHideBrowserNetworkRequest(
    content::BrowserContext* context,
    const WebRequestInfo& request) const {
  const url::Origin origin = url::Origin::Create(request.url);
  const base::StringPiece path = request.url.path_piece();
  if (((origin.DomainIs("sandbox.uphold.com") ||
        origin.DomainIs("uphold.com")) &&
       base::StartsWith(path, "/authorize/",
                        base::CompareCase::INSENSITIVE_ASCII)) ||
      (origin.DomainIs("api.uphold.com") &&
       base::StartsWith(path, "/oauth2/token",
                        base::CompareCase::INSENSITIVE_ASCII))) {
    return true;  // protected URL
  }
  return ChromeExtensionsAPIClient::ShouldHideBrowserNetworkRequest(context,
                                                                    request);
}

}  // namespace extensions
