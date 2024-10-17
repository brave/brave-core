/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/component_extensions_allowlist/allowlist.h"

#define IsComponentExtensionAllowlisted IsComponentExtensionAllowlisted_ChromiumImpl  // NOLINT
#include "src/chrome/browser/extensions/component_extensions_allowlist/allowlist.cc"
#undef IsComponentExtensionAllowlisted

#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/components/brave_extension/grit/brave_extension.h"
#include "brave/components/brave_webtorrent/grit/brave_webtorrent_resources.h"
#include "components/grit/brave_components_resources.h"
#include "extensions/common/constants.h"

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
#include "brave/browser/ethereum_remote_client/ethereum_remote_client_constants.h"
#endif

namespace extensions {

  bool IsComponentExtensionAllowlisted(const std::string& extension_id) {
    const char* const kAllowed[] = {
      brave_extension_id,
#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED)
      kEthereumRemoteClientExtensionId,
#endif
      brave_webtorrent_extension_id
    };

    for (size_t i = 0; i < std::size(kAllowed); ++i) {
      if (extension_id == UNSAFE_TODO(kAllowed[i])) {
        return true;
      }
    }

    return IsComponentExtensionAllowlisted_ChromiumImpl(extension_id);
  }

  bool IsComponentExtensionAllowlisted(int manifest_resource_id) {
    switch (manifest_resource_id) {
      // Please keep the list in alphabetical order.
      case IDR_BRAVE_EXTENSION:
      case IDR_BRAVE_WEBTORRENT:
        return true;
    }

    return IsComponentExtensionAllowlisted_ChromiumImpl(manifest_resource_id);
  }

}  // namespace extensions
