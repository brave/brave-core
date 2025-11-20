/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This helper defines CHROMIUM_INDEX for Brave builds.
// Brave uses different install mode indices than Chromium.
// CHROMIUM_INDEX doesn't exist in Brave's chromium_install_modes.h.
// For official Brave builds, the primary mode is STABLE_INDEX.
// For development builds, the primary mode is DEVELOPER_INDEX.
//
// Usage:
//  #include "brave/installer/chromium_index_helper_begin.h"
//  #include <chrome/installer/.../original_file.cc>
//  #include "brave/installer/chromium_index_helper_end.h"

#ifndef BRAVE_INSTALLER_CHROMIUM_INDEX_HELPER_BEGIN_H_
#define BRAVE_INSTALLER_CHROMIUM_INDEX_HELPER_BEGIN_H_

#include "build/branding_buildflags.h"

#if !BUILDFLAG(GOOGLE_CHROME_BRANDING) && \
    !BUILDFLAG(GOOGLE_CHROME_FOR_TESTING_BRANDING)

#if defined(OFFICIAL_BUILD)
#define CHROMIUM_INDEX STABLE_INDEX
#else
#define CHROMIUM_INDEX DEVELOPER_INDEX
#endif

#endif  // !BUILDFLAG(GOOGLE_CHROME_BRANDING) &&
        // !BUILDFLAG(GOOGLE_CHROME_FOR_TESTING_BRANDING)

#endif  // BRAVE_INSTALLER_CHROMIUM_INDEX_HELPER_BEGIN_H_
