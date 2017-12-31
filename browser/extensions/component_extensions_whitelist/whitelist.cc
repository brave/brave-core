/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/component_extensions_whitelist/whitelist.h"

#include "base/macros.h"
#include "chrome/browser/extensions/component_extensions_whitelist/whitelist.h"
#include "components/grit/brave_components_resources.h"

namespace extensions {

bool IsBraveComponentExtensionWhitelisted(const std::string& extension_id) {
  const char* const kAllowed[] = {
    "mnojpmjdmbbfmejpflffifhffcmidifd" // Brave extension ID
  };

  for (size_t i = 0; i < arraysize(kAllowed); ++i) {
    if (extension_id == kAllowed[i])
      return true;
  }

  if (IsComponentExtensionWhitelisted(extension_id)) {
    return true;
  }

  return false;
}

bool IsBraveComponentExtensionWhitelisted(int manifest_resource_id) {

  switch (manifest_resource_id) {
    // Please keep the list in alphabetical order.
    case IDR_BRAVE_EXTENSON:
      return true;
  }

  if (IsComponentExtensionWhitelisted(manifest_resource_id)) {
    return true;
  }

  return false;
}

}  // namespace extensions
