/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_EXTENSIONS_API_CLIENT_H_
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_EXTENSIONS_API_CLIENT_H_

#include "chrome/browser/extensions/api/chrome_extensions_api_client.h"
#include "extensions/browser/api/web_request/web_request_info.h"

namespace extensions {

class BraveExtensionsAPIClient : public ChromeExtensionsAPIClient {
  bool ShouldHideBrowserNetworkRequest(
      content::BrowserContext* context,
      const WebRequestInfo& request) const override;

  friend class BraveExtensionsAPIClientTests;
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_EXTENSIONS_API_CLIENT_H_
