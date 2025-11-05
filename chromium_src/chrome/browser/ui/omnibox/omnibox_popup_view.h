// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_POPUP_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_POPUP_VIEW_H_

class BraveOmniboxResultView;

#define controller_ \
  controller_;      \
  friend class BraveOmniboxResultView

#include <chrome/browser/ui/omnibox/omnibox_popup_view.h>  // IWYU pragma: export

#undef controller_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_OMNIBOX_OMNIBOX_POPUP_VIEW_H_
