/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <extensions/browser/extensions_browser_client.cc>

namespace extensions {

// Default implementation for the virtual added via
// BRAVE_EXTENSIONS_BROWSER_CLIENT_H. Overridden by
// BraveExtensionsBrowserClientImpl to consult Brave's local malware list.
bool ExtensionsBrowserClient::IsOnBraveMalwareExtensionList(
    const ExtensionId& extension_id) const {
  return false;
}

}  // namespace extensions
