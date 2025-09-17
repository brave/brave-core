/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_CONSTANTS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_CONSTANTS_H_

// All includes from the upstream's file to avoid possibility of side effects
#include "base/files/file_path.h"
#include "base/time/time.h"
#include "build/build_config.h"

// Pretend we are not Android to get constants not supposed to use on Android
// since cr142
#if BUILDFLAG(IS_ANDROID)
#undef BUILDFLAG_INTERNAL_IS_ANDROID
#define BUILDFLAG_INTERNAL_IS_ANDROID() (0)
#define IS_ANDROID_FORCED_TO_FALSE 1
#endif

#include <components/password_manager/core/browser/password_manager_constants.h>  // IWYU pragma: export

#if IS_ANDROID_FORCED_TO_FALSE
#undef BUILDFLAG_INTERNAL_IS_ANDROID
#define BUILDFLAG_INTERNAL_IS_ANDROID() (1)
#endif

#undef IS_ANDROID_FORCED_TO_FALSE

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_CONSTANTS_H_
