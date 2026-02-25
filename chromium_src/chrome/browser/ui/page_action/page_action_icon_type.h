// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_ACTION_PAGE_ACTION_ICON_TYPE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_ACTION_PAGE_ACTION_ICON_TYPE_H_

#include <chrome/browser/ui/page_action/page_action_icon_type.h>  // IWYU pragma: export

// Add a wrapper function to the Chromium implementation to avoid conflicts.
bool IsPageActionMigrated_Chromium(PageActionIconType page_action);

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_PAGE_ACTION_PAGE_ACTION_ICON_TYPE_H_
