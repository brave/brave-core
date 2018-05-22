/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extension_constants.h"
#include "components/grit/brave_components_resources.h"

#define IsComponentExtensionWhitelisted IsComponentExtensionWhitelistedInternal
#include "../../../../../../chrome/browser/extensions/component_extensions_whitelist/whitelist.cc"
#undef IsComponentExtensionWhitelisted


namespace extensions {

bool IsComponentExtensionWhitelisted(int manifest_resource_id) {
  switch (manifest_resource_id) {
    case IDR_BRAVE_EXTENSON:
    case IDR_BRAVE_BRAVE_DARK_THEME:
    case IDR_BRAVE_BRAVE_LIGHT_THEME:
      return true;
  }
  return IsComponentExtensionWhitelistedInternal(manifest_resource_id);
}

bool IsComponentExtensionWhitelisted(const std::string& extension_id) {
  const char* const kAllowed[] = {
    brave_extension_id,
    brave_dark_theme_id,
    brave_light_theme_id,
  };

  for (size_t i = 0; i < arraysize(kAllowed); ++i) {
    if (extension_id == kAllowed[i])
      return true;
  }

  return IsComponentExtensionWhitelistedInternal(extension_id);
}

}  // extensions
