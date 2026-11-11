/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/extensions/component_loader.h"
#include "extensions/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/extensions/brave_component_loader.h"

using CurrentComponentLoader = extensions::BraveComponentLoader;
#else
// BraveComponentLoader only loads the Brave extension, which is desktop-only.
using CurrentComponentLoader = extensions::ComponentLoader;
#endif  // BUILDFLAG(ENABLE_EXTENSIONS)

#include <chrome/browser/extensions/component_loader_factory.cc>
