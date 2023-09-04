/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/extension_util.h"

#define GetInstallPromptPermissionSetForExtension \
  GetInstallPromptPermissionSetForExtension_ChromiumImpl
#include "src/chrome/browser/extensions/extension_util.cc"  // IWYU pragma: export
#undef GetInstallPromptPermissionSetForExtension

#include "extensions/common/constants.h"

namespace extensions {
namespace util {
std::unique_ptr<const PermissionSet> GetInstallPromptPermissionSetForExtension(
    const Extension* extension,
    Profile* profile,
    bool include_optional_permissions) {
  auto permissions_to_display =
      GetInstallPromptPermissionSetForExtension_ChromiumImpl(
          extension, profile, include_optional_permissions);

  if (permissions_to_display &&
      (extension->id() == ipfs_companion_extension_id ||
       extension->id() == ipfs_companion_beta_extension_id)) {
    APIPermissionSet api_permission_set;
    if (permissions_to_display->apis().find(mojom::APIPermissionID::kIpfs) ==
        permissions_to_display->apis().end()) {
      api_permission_set.insert(mojom::APIPermissionID::kIpfs);
    }
    api_permission_set.insert(mojom::APIPermissionID::kIpfsPrivate);
    PermissionSet permission_set =
        PermissionSet(std::move(api_permission_set), ManifestPermissionSet(),
                      URLPatternSet(), URLPatternSet());
    permissions_to_display =
        PermissionSet::CreateUnion(*permissions_to_display, permission_set);
  }

  return permissions_to_display;
}

}  // namespace util
}  // namespace extensions
