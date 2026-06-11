// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/browser_tabrestore.h"

#include "brave/browser/ui/tabs/tree_tab_session_manager.h"
#include "brave/components/containers/buildflags/buildflags.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"

#if BUILDFLAG(ENABLE_CONTAINERS)
#include "brave/components/containers/content/browser/tab_restore_utils.h"
#endif  // BUILDFLAG(ENABLE_CONTAINERS)

namespace {

content::WebContents* MaybeRestoreTabTreeHierarchy(
    Browser* browser,
    content::WebContents* restored_web_contents,
    const std::map<std::string, std::string>& extra_data) {
  browser->browser_window_features()
      ->GetTreeTabSessionManager()
      ->MaybeRestoreTabTreeHierarchy(restored_web_contents, extra_data);
  return restored_web_contents;
}

}  // namespace

#include <chrome/browser/ui/browser_tabrestore.cc>
