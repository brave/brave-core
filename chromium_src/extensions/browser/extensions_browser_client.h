/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_EXTENSIONS_BROWSER_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_EXTENSIONS_BROWSER_CLIENT_H_

// Adds a virtual so Brave can augment the extension blocklist with its
// distributed malicious-extension list. Overridden by
// BraveExtensionsBrowserClientImpl; the default is false so upstream/other
// embedders are unaffected.
#define BRAVE_EXTENSIONS_BROWSER_CLIENT_H                                     \
  virtual bool IsOnBraveMalwareExtensionList(const ExtensionId& extension_id) \
      const;

#include <extensions/browser/extensions_browser_client.h>  // IWYU pragma: export

#undef BRAVE_EXTENSIONS_BROWSER_CLIENT_H

#endif  // BRAVE_CHROMIUM_SRC_EXTENSIONS_BROWSER_EXTENSIONS_BROWSER_CLIENT_H_
