/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_EXTENSIONS_COMMON_PERMISSIONS_BRAVE_PERMISSIONS_DATA_H_
#define BRAVE_EXTENSIONS_COMMON_PERMISSIONS_BRAVE_PERMISSIONS_DATA_H_

#include "extensions/common/extension_id.h"
#include "extensions/common/manifest.h"
#include "extensions/common/permissions/permission_set.h"
#include "extensions/common/permissions/permissions_data.h"
#include "extensions/common/url_pattern_set.h"
#include "url/gurl.h"

namespace extensions {

class BravePermissionsData : public PermissionsData {
public:
  BravePermissionsData(const ExtensionId& extension_id,
                       Manifest::Type manifest_type,
                       Manifest::Location location,
                       std::unique_ptr<const PermissionSet> initial_permissions);

protected:
  // Returns whether or not the extension is permitted to run on the given page,
  // checking against |permitted_url_patterns| and |tab_url_patterns| in
  // addition to blocking special sites (like the webstore or chrome:// urls).
  // Must be called with |runtime_lock_| acquired.
  PermissionsData::PageAccess CanRunOnPage(const GURL& document_url,
      int tab_id, const URLPatternSet& permitted_url_patterns,
      const URLPatternSet& withheld_url_patterns,
      const URLPatternSet* tab_url_patterns, std::string* error)
      const override;

private:
  static const char kCannotScriptWalletLinking[];
};

}  // namespace extensions

#endif  // BRAVE_EXTENSIONS_COMMON_PERMISSIONS_BRAVE_PERMISSIONS_DATA_H_
