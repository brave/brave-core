/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/about_flags.cc"
#include "brave/components/commander/common/buildflags/buildflags.h"
#include "chrome/common/channel_info.h"
#include "components/autofill/core/browser/autofill_experiments.h"

#if BUILDFLAG(ENABLE_COMMANDER)
#include "brave/components/commander/common/features.h"

// Note: We replace the kQuickCommands feature with the kBraveCommander feature
// so we can use it from //components without DEPS violations.
#define kQuickCommands kBraveCommander
#endif

#define BRAVE_SHOULD_SKIP_CONDITIONAL_FEATURE_ENTRY                       \
  if (flags_ui::BraveShouldSkipConditionalFeatureEntry(storage, entry)) { \
    return true;                                                          \
  }

#include "src/chrome/browser/about_flags.cc"
#undef BRAVE_SHOULD_SKIP_CONDITIONAL_FEATURE_ENTRY
#undef kQuickCommands
