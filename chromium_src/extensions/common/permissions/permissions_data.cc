/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "extensions/common/permissions/permissions_data.h"

#include "brave/components/skus/common/skus_utils.h"

namespace {

bool IsBraveProtectedUrl(const GURL& url) {
  if (skus::IsSafeOrigin(url)) {
    return true;
  }
  return false;
}

bool IsBraveRestrictedUrl(const GURL& document_url,
                          const extensions::ExtensionId& extension_id,
                          extensions::mojom::ManifestLocation location,
                          std::string* error) {
  if (extensions::PermissionsData::CanExecuteScriptEverywhere(extension_id,
                                                              location)) {
    return false;
  }

  if (IsBraveProtectedUrl(document_url)) {
    return true;
  }

  return false;
}

}  // namespace

// Disable some content scripts until users click on the extension icon
#define BRAVE_PERMISSION_DATA_CAN_RUN_ON_PAGE                                \
  if (IsBraveRestrictedUrl(document_url, extension_id_, location_, error)) { \
    return PageAccess::kWithheld;                                            \
  }

#include "src/extensions/common/permissions/permissions_data.cc"

#undef BRAVE_PERMISSION_DATA_CAN_RUN_ON_PAGE
