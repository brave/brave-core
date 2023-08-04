/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/about_flags.cc"
#include "brave/components/commander/common/buildflags/buildflags.h"

#if BUILDFLAG(ENABLE_COMMANDER)
#include "brave/components/commander/common/features.h"

// Note: We replace the kQuickCommands feature with the kBraveCommander feature
// so we can use it from //components without DEPS violations.
#define kQuickCommands kBraveCommander
#endif

#include "src/chrome/browser/about_flags.cc"
#undef kQuickCommands
