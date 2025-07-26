// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_OBSERVER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_OBSERVER_H_

#define TabPinnedStateChanged                               \
  TabCustomTitleChanged(content::WebContents* contents,     \
                        const std::string& custom_title) {} \
  virtual void TabPinnedStateChanged

#include <chrome/browser/ui/tabs/tab_strip_model_observer.h>  // IWYU pragma: export

#undef TabPinnedStateChanged

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_OBSERVER_H_
