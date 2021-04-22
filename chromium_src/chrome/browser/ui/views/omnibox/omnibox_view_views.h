/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_VIEW_VIEWS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_VIEW_VIEWS_H_

#define OnOmniboxPaste                \
  NotUsed();                          \
  friend class BraveOmniboxViewViews; \
  void OnOmniboxPaste

#include "../../../../../../../chrome/browser/ui/views/omnibox/omnibox_view_views.h"

#undef OnOmniboxPaste

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_OMNIBOX_OMNIBOX_VIEW_VIEWS_H_
