/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BUBBLE_WEBUI_BUBBLE_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BUBBLE_WEBUI_BUBBLE_MANAGER_H_

// `WebUIBubbleManager` is patched to include a new virtual method that is
// called after a `WebUIBubbleDialogView` is created and before it is supplied
// to `views::BubbleDialogDelegateView::CreateBubble`. This allows us to
// customize the view appropriately (e.g. by setting the border radius) prior to
// rendering. For a class that uses this method, see `BraveWebUIBubbleManager`.

#define DisableCloseBubbleHelperForTesting()    \
  DisableCloseBubbleHelperForTesting_NotUsed(); \
  virtual void BraveCustomizeBubbleDialogView(  \
      WebUIBubbleDialogView& bubble_view) {}    \
  void DisableCloseBubbleHelperForTesting()

#define BRAVE_WEBUI_BUBBLE_MANAGER_T_CREATE_WEB_UI_BUBBLE_DIALOG \
  BraveCustomizeBubbleDialogView(*bubble_view);

#include "src/chrome/browser/ui/views/bubble/webui_bubble_manager.h"  // IWYU pragma: export

#undef BRAVE_WEBUI_BUBBLE_MANAGER_T_CREATE_WEB_UI_BUBBLE_DIALOG
#undef DisableCloseBubbleHelperForTesting

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_BUBBLE_WEBUI_BUBBLE_MANAGER_H_
