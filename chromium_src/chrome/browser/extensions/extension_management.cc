/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "extensions/buildflags/buildflags.h"

// Android uses its own BraveExtensionManagement implementation; include the
// matching header to avoid an ODR violation.
#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/extensions/brave_extension_management.h"
#elif BUILDFLAG(ENABLE_EXTENSIONS_CORE)
#include "brave/browser/extensions/android/brave_extension_management.h"
#endif

#include <chrome/browser/extensions/extension_management.cc>
