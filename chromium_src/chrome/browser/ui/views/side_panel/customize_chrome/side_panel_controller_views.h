// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_CUSTOMIZE_CHROME_SIDE_PANEL_CONTROLLER_VIEWS_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_CUSTOMIZE_CHROME_SIDE_PANEL_CONTROLLER_VIEWS_H_

#include "base/memory/weak_ptr.h"

#define will_discard_contents_callback_subscription_                 \
  will_discard_contents_callback_subscription_;                      \
  base::WeakPtrFactory<SidePanelControllerViews> weak_ptr_factory_ { \
    this                                                             \
  }

#include "src/chrome/browser/ui/views/side_panel/customize_chrome/side_panel_controller_views.h"  // IWYU pragma: export

#undef will_discard_contents_callback_subscription_

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SIDE_PANEL_CUSTOMIZE_CHROME_SIDE_PANEL_CONTROLLER_VIEWS_H_
