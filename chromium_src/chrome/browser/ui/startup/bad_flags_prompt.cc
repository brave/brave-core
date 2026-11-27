// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/startup/bad_flags_prompt.h"

#include "build/build_config.h"

#if !BUILDFLAG(IS_ANDROID)
#include "brave/brave_domains/constants.h"
#endif

#define ShowBadFlagsPrompt ShowBadFlagsPrompt_ChromiumImpl
#include <chrome/browser/ui/startup/bad_flags_prompt.cc>
#undef ShowBadFlagsPrompt

void ShowBadFlagsPrompt(content::WebContents* web_contents) {
#if !BUILDFLAG(IS_ANDROID)
  for (const auto& [name, _] :
       base::CommandLine::ForCurrentProcess()->GetSwitches()) {
    if (name.starts_with(brave_domains::kBraveServicesOverrideSwitchPrefix)) {
      ShowBadFlagsInfoBar(web_contents, IDS_BAD_FLAGS_WARNING_MESSAGE, name);
      return;
    }
  }
#endif

  ShowBadFlagsPrompt_ChromiumImpl(web_contents);
}
