/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/installer/util/util_constants.h"

#define kChromeExe kChromeExe_Unused
#include <chrome/installer/util/util_constants.cc>
#undef kChromeExe

namespace installer {

namespace switches {

// Useful only when used with --update-setup-exe; otherwise ignored. Specifies
// the full path where the updated setup.exe will be written. Any other files
// created in the indicated directory may be deleted by the caller after process
// termination.
const char kNewSetupExe[] = "new-setup-exe";

// Provide the previous version that patch is for.
const char kPreviousVersion[] = "previous-version";

// Also see --new-setup-exe. This command line option specifies a diff patch
// that setup.exe will apply to itself and store the resulting binary in the
// path given by --new-setup-exe.
const char kUpdateSetupExe[] = "update-setup-exe";

}  // namespace switches

const wchar_t kChromeExe[] = L"brave.exe";

}  // namespace installer
