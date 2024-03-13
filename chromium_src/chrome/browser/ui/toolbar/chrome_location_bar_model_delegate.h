/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TOOLBAR_CHROME_LOCATION_BAR_MODEL_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TOOLBAR_CHROME_LOCATION_BAR_MODEL_DELEGATE_H_

#define GetNavigationEntry                    \
  GetNavigationEntryUnused();                 \
  friend class BraveLocationBarModelDelegate; \
  content::NavigationEntry* GetNavigationEntry

#include "src/chrome/browser/ui/toolbar/chrome_location_bar_model_delegate.h"  // IWYU pragma: export
#undef GetNavigationEntry

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TOOLBAR_CHROME_LOCATION_BAR_MODEL_DELEGATE_H_
