/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_SPLIT_TAB_MENU_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_SPLIT_TAB_MENU_MODEL_H_

#include "base/gtest_prod_util.h"

// Export GetCommandIdXXX() functions from cc to call it in subclass and test.
#define CloseTabAtIndex(...)                                \
  CloseTabAtIndex(__VA_ARGS__);                             \
  friend class BraveSplitTabMenuModel;                      \
  FRIEND_TEST_ALL_PREFIXES(BraveTabContextMenuContentsTest, \
                           SplitViewMenuCustomizationTest); \
  static CommandId GetCommandIdEnum(int command_id);        \
  static int GetCommandIdInt(CommandId command_id)

#define GetReversePositionIcon(...) \
  GetReversePosition_UnUsed() {     \
    NOTREACHED();                   \
  }                                 \
  virtual const gfx::VectorIcon& GetReversePositionIcon(__VA_ARGS__)

#define kExitSplit kExitSplit, kToggleLinkState

#include <chrome/browser/ui/tabs/split_tab_menu_model.h>  // IWYU pragma: export

#undef kExitSplit
#undef GetReversePositionIcon
#undef CloseTabAtIndex

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_SPLIT_TAB_MENU_MODEL_H_
