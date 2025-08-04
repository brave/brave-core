/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/about_flags.cc"

#include "chrome/common/channel_info.h"
#include "components/autofill/core/browser/studies/autofill_experiments.h"

#define BRAVE_SHOULD_SKIP_CONDITIONAL_FEATURE_ENTRY                      \
  /* Only show AI Agentic Profile flag on Nightly and Local */           \
  if (!strcmp(kAIChatAgenticProfileInternalName, entry.internal_name)) { \
    version_info::Channel chrome_channel = chrome::GetChannel();         \
    return chrome_channel != version_info::Channel::DEV &&               \
           chrome_channel != version_info::Channel::CANARY &&            \
           chrome_channel != version_info::Channel::UNKNOWN;             \
  }

#include <chrome/browser/about_flags.cc>
#undef BRAVE_SHOULD_SKIP_CONDITIONAL_FEATURE_ENTRY
