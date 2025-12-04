/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/installer/setup/brave_setup_util.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/version.h"
#include "chrome/installer/setup/installer_state.h"
#include "chrome/installer/setup/setup_constants.h"
#include "chrome/installer/setup/setup_util.h"
#include "chrome/installer/util/installation_state.h"

namespace installer {

// This function implementation used to be upstream and had to be restored in
// Brave to support delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937
base::FilePath FindArchiveToPatch(const InstallationState& original_state,
                                  const InstallerState& installer_state,
                                  const base::Version& desired_version) {
  if (desired_version.IsValid()) {
    base::FilePath archive(
        installer_state.GetInstallerDirectory(desired_version)
            .Append(kChromeArchive));
    return base::PathExists(archive) ? archive : base::FilePath();
  }

  // Check based on the version number advertised to Google Update, since that
  // is the value used to select a specific differential update. If an archive
  // can't be found using that, fallback to using the newest version present.
  base::FilePath patch_source;
  const ProductState* product =
      original_state.GetProductState(installer_state.system_install());
  if (product) {
    patch_source = installer_state.GetInstallerDirectory(product->version())
                       .Append(installer::kChromeArchive);
    if (base::PathExists(patch_source)) {
      return patch_source;
    }
  }
  std::unique_ptr<base::Version> version(
      installer::GetMaxVersionFromArchiveDir(installer_state.target_path()));
  if (version) {
    patch_source = installer_state.GetInstallerDirectory(*version).Append(
        installer::kChromeArchive);
    if (base::PathExists(patch_source)) {
      return patch_source;
    }
  }
  return base::FilePath();
}

}  // namespace installer
