/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Brand-specific constants and install modes for Brave.

#include <stdlib.h>

#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "chrome/install_static/install_modes.h"

namespace install_static {

const wchar_t kCompanyPathName[] = L"BraveSoftware";

#if defined(OFFICIAL_BUILD)
#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
// Brave Origin uses "Brave-Origin" instead of "Brave-Browser" to allow
// side-by-side installation with Brave Browser.
const wchar_t kProductPathName[] = L"Brave-Origin";
#else
const wchar_t kProductPathName[] = L"Brave-Browser";
#endif  // BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
#else
// If you change this, then you also need to change occurrences of this string
// in mini_installer_constants.cc.
const wchar_t kProductPathName[] = L"Brave-Browser-Development";
#endif

const size_t kProductPathNameLength = _countof(kProductPathName) - 1;

constexpr char kSafeBrowsingName[] = "chromium";

}  // namespace install_static
