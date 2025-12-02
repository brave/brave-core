/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Brand-specific constants and install modes for Brave.

#include <stdlib.h>

#include "chrome/install_static/install_modes.h"

namespace install_static {

const wchar_t kCompanyPathName[] = L"BraveSoftware";

#if defined(OFFICIAL_BUILD)
const wchar_t kProductPathName[] = L"Brave-Browser";
#else
const wchar_t kProductPathName[] = L"Brave-Browser-Development";
#endif

const size_t kProductPathNameLength = _countof(kProductPathName) - 1;

constexpr char kSafeBrowsingName[] = "chromium";

}  // namespace install_static
