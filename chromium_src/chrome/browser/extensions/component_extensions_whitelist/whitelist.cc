/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#define IsComponentExtensionWhitelisted IsComponentExtensionWhitelisted_ChromiumImpl  // NOLINT
#include "../../../../../../chrome/browser/extensions/component_extensions_whitelist/whitelist.cc"  // NOLINT
#undef IsComponentExtensionWhitelisted

#include "base/stl_util.h"
#include "brave/common/extensions/extension_constants.h"
#include "brave/components/brave_extension/grit/brave_extension.h"
#include "components/grit/brave_components_resources.h"
#include "brave/components/brave_rewards/resources/extension/grit/brave_rewards_extension_resources.h"
#include "brave/components/brave_sync/grit/brave_sync_resources.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_resources.h"

namespace extensions {

  bool IsComponentExtensionWhitelisted(const std::string& extension_id) {
    const char* const kAllowed[] = {
      brave_extension_id,
      brave_rewards_extension_id,
      brave_sync_extension_id,
      ethereum_remote_client_extension_id,
      brave_webtorrent_extension_id
    };

    for (size_t i = 0; i < base::size(kAllowed); ++i) {
      if (extension_id == kAllowed[i])
        return true;
    }

    return IsComponentExtensionWhitelisted_ChromiumImpl(extension_id);
  }

  bool IsComponentExtensionWhitelisted(int manifest_resource_id) {
    switch (manifest_resource_id) {
      // Please keep the list in alphabetical order.
      case IDR_BRAVE_EXTENSION:
      case IDR_BRAVE_REWARDS:
      case IDR_BRAVE_SYNC_EXTENSION:
      case IDR_BRAVE_WEBTORRENT:
        return true;
    }

    return IsComponentExtensionWhitelisted_ChromiumImpl(manifest_resource_id);
  }

}  // namespace extensions
