/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#define CanRunOnPage CanRunOnPage_ChromiumImpl
#include "../../../../../extensions/common/permissions/permissions_data.cc"
#undef CanRunOnPage

#include "brave/extensions/common/brave_extension_urls.h"

namespace extensions {

PermissionsData::PageAccess PermissionsData::CanRunOnPage(
    const GURL& document_url,
    int tab_id,
    const URLPatternSet& permitted_url_patterns,
    const URLPatternSet& withheld_url_patterns,
    const URLPatternSet* tab_url_patterns,
    std::string* error) const {
  return PageAccess::kDenied;  // TODO: remove this!
  PermissionsData::PageAccess access = CanRunOnPage_ChromiumImpl(document_url,
      tab_id, permitted_url_patterns, withheld_url_patterns, tab_url_patterns,
      error);
  if (access != PageAccess::kAllowed) {
    return access;
  }

  if (CanExecuteScriptEverywhere(extension_id_, location_)) {
    return PageAccess::kAllowed;
  }

  if (extension_urls::IsBraveProtectedUrl(url::Origin::Create(document_url),
                                          document_url.path_piece())) {
    // Disable content scripts until users click on the extension.
    return PageAccess::kWithheld;
  }

  return PageAccess::kAllowed;
}

}  // namespace extensions
