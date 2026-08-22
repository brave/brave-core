/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/brave_extensions_browser_client_impl.h"

#include <memory>
#include <tuple>

#include "base/check.h"
#include "brave/browser/extensions/brave_extensions_browser_api_provider.h"
#include "brave/components/extension_malware_blocklist/browser/extension_malware_blocklist.h"
#include "chrome/browser/extensions/chrome_component_extension_resource_manager.h"

namespace extensions {

BraveExtensionsBrowserClientImpl::BraveExtensionsBrowserClientImpl() {
  AddAPIProvider(std::make_unique<BraveExtensionsBrowserAPIProvider>());
}

bool BraveExtensionsBrowserClientImpl::IsOnBraveMalwareExtensionList(
    const ExtensionId& extension_id) const {
  auto* blocklist =
      extension_malware_blocklist::ExtensionMalwareBlocklist::GetInstance();
  return blocklist && blocklist->IsMalware(extension_id);
}

}  // namespace extensions
