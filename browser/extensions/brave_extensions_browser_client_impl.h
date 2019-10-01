/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSIONS_BROWSER_CLIENT_IMPL_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSIONS_BROWSER_CLIENT_IMPL_H_

#include "brave/extensions/browser/brave_extensions_browser_client.h"
#include "chrome/browser/extensions/chrome_extensions_browser_client.h"

namespace extensions {

class BraveExtensionsBrowserClientImpl : public ChromeExtensionsBrowserClient,
                                         public BraveExtensionsBrowserClient {
 public:
  BraveExtensionsBrowserClientImpl();
  ~BraveExtensionsBrowserClientImpl() override = default;

  DISALLOW_COPY_AND_ASSIGN(BraveExtensionsBrowserClientImpl);
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_EXTENSIONS_BROWSER_CLIENT_IMPL_H_
