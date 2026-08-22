// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "content/public/browser/web_contents.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/preserve_container_destination.h"
#endif

void BraveAdjustLoadURLParams([[maybe_unused]] NavigateParams* params,
                              [[maybe_unused]] content::WebContents* contents) {
#if BUILDFLAG(ENABLE_CONTAINERS)
  if (params->preserve_container_destination && contents) {
    containers::PreserveContainerDestination::CreateForWebContents(contents);
  }
#endif
}
