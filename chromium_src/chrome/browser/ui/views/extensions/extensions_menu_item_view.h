/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSIONS_MENU_ITEM_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSIONS_MENU_ITEM_VIEW_H_

#define OnPinButtonPressed                 \
  OnPinButtonPressed_Unused();             \
  friend class BraveExtensionMenuItemView; \
  void OnPinButtonPressed

#define UpdateContextMenuButton virtual UpdateContextMenuButton
#define UpdatePinButton virtual UpdatePinButton

#include "src/chrome/browser/ui/views/extensions/extensions_menu_item_view.h"  // IWYU pragma: export

#undef UpdatePinButton
#undef UpdateContextMenuButton

#undef OnPinButtonPressed

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSIONS_MENU_ITEM_VIEW_H_
