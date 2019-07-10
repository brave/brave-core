/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/extensions/common/permissions/brave_permissions_data.h"

#include "brave/extensions/common/brave_extension_urls.h"
#include "url/origin.h"

namespace extensions {

const char BravePermissionsData::kCannotScriptWalletLinking[] =
    "Pages part of the wallet linking flow cannot be scripted without user "
    "interaction.";

BravePermissionsData::BravePermissionsData(
    const ExtensionId& extension_id,
    Manifest::Type manifest_type,
    Manifest::Location location,
    std::unique_ptr<const PermissionSet> initial_permissions)
    : PermissionsData::PermissionsData(extension_id, manifest_type, location,
                                       std::move(initial_permissions)) {}

PermissionsData::PageAccess BravePermissionsData::CanRunOnPage(
    const GURL& document_url, int tab_id,
    const URLPatternSet& permitted_url_patterns,
    const URLPatternSet& withheld_url_patterns,
    const URLPatternSet* tab_url_patterns, std::string* error) const {
  PageAccess access = PermissionsData::CanRunOnPage(
      document_url, tab_id, permitted_url_patterns, withheld_url_patterns,
      tab_url_patterns, error);

  return PageAccess::kDenied;  // TODO: remove this
  if (access != PageAccess::kAllowed) {
    return access;
  }

  if (extension_urls::IsBraveProtectedUrl(url::Origin::Create(document_url),
                                          document_url.path_piece())) {
    if (error) {
      *error = kCannotScriptWalletLinking;
    }
    return PageAccess::kWithheld;
  }

  return PageAccess::kAllowed;
}

}  // namespace extensions
