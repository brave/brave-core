/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_MENU_MODEL_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_MENU_MODEL_DELEGATE_H_

namespace containers {
class ContainersMenuModelDelegate;
}  // namespace containers

// Extend the original class with brave interfaces for tab context menu.
// To make subclass buildable, new api's default implementation is provided
// in brave/browser/ui/tabs/tab_menu_model_delegate.cc.
// Last |unused_| is added to handle remained " = 0;".
#define GetTabGroupSyncService(...)                \
  GetTabGroupSyncService(__VA_ARGS__) = 0;         \
  virtual bool ShouldShowVerticalTab();            \
  virtual containers::ContainersMenuModelDelegate* \
  GetContainersMenuModelDelegate();                \
  const int unused_

#include <chrome/browser/ui/tabs/tab_menu_model_delegate.h>  // IWYU pragma: export

#undef GetTabGroupSyncService

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_MENU_MODEL_DELEGATE_H_
