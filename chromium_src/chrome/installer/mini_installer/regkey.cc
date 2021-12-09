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

#include "src/chrome/installer/mini_installer/regkey.cc"

#if defined(OFFICIAL_BUILD)
#undef BUILDFLAG_INTERNAL_GOOGLE_CHROME_BRANDING
#endif
