/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_BRAVE_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_
#define BRAVE_BROWSER_EXTENSIONS_BRAVE_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_

#include "chrome/browser/extensions/chrome_component_extension_resource_manager.h"

namespace extensions {

class BraveComponentExtensionResourceManager
    : public ChromeComponentExtensionResourceManager {
 public:
  BraveComponentExtensionResourceManager();
  ~BraveComponentExtensionResourceManager() override;

  DISALLOW_COPY_AND_ASSIGN(BraveComponentExtensionResourceManager);
};

}  // namespace extensions

#endif  // BRAVE_BROWSER_EXTENSIONS_BRAVE_COMPONENT_EXTENSION_RESOURCE_MANAGER_H_
