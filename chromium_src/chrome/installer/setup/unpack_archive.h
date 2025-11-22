/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_SETUP_UNPACK_ARCHIVE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_SETUP_UNPACK_ARCHIVE_H_

// N.B. This file does not #include upstream's implementation.

#include "base/types/expected.h"
#include "chrome/installer/util/util_constants.h"

namespace base {
class CommandLine;
class FilePath;
}  // namespace base

namespace installer {

class InstallationState;
class InstallerState;

// The sole purpose of this declaration is to change the installer_state
// parameter from const to non-const. This lets us stuff return values into
// installer_state in a way that otherwise preserves upstream's function
// signatures. Specifically:
//
// This function used to have additional parameters ArchiveType* archive_type
// and base::FilePath& uncompressed_archive. Upstream removed them when it
// moved the logic for differential updates into Omaha 4. But we are still on
// Omaha 3 on Windows and therefore need to preserve the old functionality. We
// do this by storing (and returning) the archive_type and uncompressed_archive
// values in the installer_state object.
base::expected<base::FilePath, InstallStatus> UnpackChromeArchive(
    const base::FilePath& unpack_path,
    InstallationState& original_state,
    const base::FilePath& setup_exe,
    const base::CommandLine& cmd_line,
    InstallerState& installer_state);

}  // namespace installer

#endif  // BRAVE_CHROMIUM_SRC_CHROME_INSTALLER_SETUP_UNPACK_ARCHIVE_H_
