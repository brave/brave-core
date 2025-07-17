/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/component_extensions_allowlist/allowlist.h"

#define IsComponentExtensionAllowlisted IsComponentExtensionAllowlisted_ChromiumImpl  // NOLINT
#include "src/chrome/browser/extensions/component_extensions_allowlist/allowlist.cc"
#undef IsComponentExtensionAllowlisted

#include "brave/components/brave_extension/grit/brave_extension.h"
#include "components/grit/brave_components_resources.h"
#include "extensions/common/constants.h"

namespace extensions {

  bool IsComponentExtensionAllowlisted(const std::string& extension_id) {
    const char* const kAllowed[] = {brave_extension_id};

    for (const auto* id : kAllowed) {
      if (extension_id == id) {
        return true;
      }
    }

    return IsComponentExtensionAllowlisted_ChromiumImpl(extension_id);
  }

  bool IsComponentExtensionAllowlisted(int manifest_resource_id) {
    switch (manifest_resource_id) {
      // Please keep the list in alphabetical order.
      case IDR_BRAVE_EXTENSION:
        return true;
    }

    return IsComponentExtensionAllowlisted_ChromiumImpl(manifest_resource_id);
  }

}  // namespace extensions
