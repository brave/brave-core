/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_COMPONENT_EXTENSIONS_WHITELIST_WHITELIST_H_
#define BRAVE_BROWSER_EXTENSIONS_COMPONENT_EXTENSIONS_WHITELIST_WHITELIST_H_

#include <string>

namespace extensions {

// Checks using an extension ID.
bool IsBraveComponentExtensionWhitelisted(const std::string& extension_id);

// Checks using resource ID of manifest.
bool IsBraveComponentExtensionWhitelisted(int manifest_resource_id);

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_COMPONENT_EXTENSIONS_WHITELIST_WHITELIST_H_
