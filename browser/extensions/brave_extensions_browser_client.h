/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSIONS_BROWSER_CLIENT_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSIONS_BROWSER_CLIENT_H_

#include "chrome/browser/extensions/chrome_extensions_browser_client.h"

namespace extensions {

class BraveExtensionsBrowserClient : public ChromeExtensionsBrowserClient {
  public:
    BraveExtensionsBrowserClient();
    ~BraveExtensionsBrowserClient() override;

    bool HasTorContext(content::BrowserContext* context) override;
    content::BrowserContext* GetTorContext(
        content::BrowserContext* context) override;

  DISALLOW_COPY_AND_ASSIGN(BraveExtensionsBrowserClient);
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSIONS_BROWSER_CLIENT_H_
