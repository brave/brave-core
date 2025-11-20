/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/installer/mini_installer/regkey.h"

#include "build/branding_buildflags.h"
#include "chrome/installer/mini_installer/mini_installer_constants.h"
#include "chrome/installer/mini_installer/mini_string.h"

#if defined(OFFICIAL_BUILD)
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#define BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING() (1)
#endif

#include <chrome/installer/mini_installer/regkey.cc>

#if defined(OFFICIAL_BUILD)
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#endif

namespace mini_installer {

// This function used to be upstream and had to be restored in Brave to support
// delta updates on Windows until we are on Omaha 4. See:
// github.com/brave/brave-core/pull/31937
LONG RegKey::WriteSZValue(const wchar_t* value_name, const wchar_t* value) {
  return ::RegSetValueEx(key_, value_name, 0, REG_SZ,
                         reinterpret_cast<const BYTE*>(value),
                         (lstrlen(value) + 1) * sizeof(wchar_t));
}

}  // namespace mini_installer
