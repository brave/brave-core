/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This helper undefines CHROMIUM_INDEX after including the original file.
// See chromium_index_helper_begin.h for usage.

#ifndef BRAVE_INSTALLER_UTIL_CHROMIUM_INDEX_HELPER_END_H_
#define BRAVE_INSTALLER_UTIL_CHROMIUM_INDEX_HELPER_END_H_

#include "build/branding_buildflags.h"

#if !BUILDFLAG(GOOGLE_CHROME_BRANDING) && \
    !BUILDFLAG(GOOGLE_CHROME_FOR_TESTING_BRANDING)
#undef CHROMIUM_INDEX
#endif

#endif  // BRAVE_INSTALLER_UTIL_CHROMIUM_INDEX_HELPER_END_H_
