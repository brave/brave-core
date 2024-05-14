/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_ATTRIBUTES_ENTRY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_ATTRIBUTES_ENTRY_H_

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
// To avoid conflicts with the macro from the Windows SDK...
#undef GetUserName
#endif

#include "src/chrome/browser/profiles/profile_attributes_entry.h"  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PROFILES_PROFILE_ATTRIBUTES_ENTRY_H_
