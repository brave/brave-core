/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/extensions/brave_component_loader.h"
#else
// BraveComponentLoader only loads the Brave extension, which is desktop-only.
#define BraveComponentLoader ComponentLoader
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

#include <chrome/browser/extensions/component_loader_factory.cc>

#if !BUILDFLAG(ENABLE_EXTENSIONS)
#undef BraveComponentLoader
#endif  // !BUILDFLAG(ENABLE_EXTENSIONS)
