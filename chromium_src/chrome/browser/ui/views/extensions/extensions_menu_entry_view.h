// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSIONS_MENU_ENTRY_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSIONS_MENU_ENTRY_VIEW_H_

#define SetupContextMenuButton               \
  SetupContextMenuButton_Unused();           \
  friend class BraveExtensionsMenuEntryView; \
  void SetupContextMenuButton

#define UpdateContextMenuButton virtual UpdateContextMenuButton

#include <chrome/browser/ui/views/extensions/extensions_menu_entry_view.h>  // IWYU pragma: export

#undef SetupContextMenuButton
#undef UpdateContextMenuButton

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_EXTENSIONS_EXTENSIONS_MENU_ENTRY_VIEW_H_
