/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser.h"
#include "brave/components/sidebar/buildflags/buildflags.h"
#include "chrome/browser/ui/browser_list.h"

#if BUILDFLAG(ENABLE_SIDEBAR)
#include "brave/browser/ui/sidebar/sidebar_controller.h"
#include "brave/browser/ui/sidebar/sidebar_model.h"
#endif

#define FindBrowserWithWebContents FindBrowserWithWebContents_ChromiumImpl

#include "../../../../../chrome/browser/ui/browser_finder.cc"

#undef FindBrowserWithWebContents

namespace chrome {

Browser* FindBrowserWithWebContents(const WebContents* web_contents) {
  DCHECK(web_contents);

#if BUILDFLAG(ENABLE_SIDEBAR)
  for (auto* browser : *BrowserList::GetInstance()) {
    // Use |sidebar_controller| directly, because even if the feature is
    // enabled, SidebarController can be not created for a specific
    // BraveBrowser.
    const auto* sidebar_controller =
        static_cast<BraveBrowser*>(browser)->sidebar_controller();
    DCHECK(!sidebar_controller || sidebar_controller->model());
    if (sidebar_controller &&
        sidebar_controller->model()->IsSidebarWebContents(web_contents)) {
      return browser;
    }
  }
#endif

  return FindBrowserWithWebContents_ChromiumImpl(web_contents);
}

}  // namespace chrome
