/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_H_

#include "third_party/abseil-cpp/absl/status/statusor.h"

#define CommandCommerceProductSpecifications \
  CommandAddTabToSuggestedGroup, CommandCommerceProductSpecifications

#define ExecuteContextMenuCommand                                        \
  ExecuteContextMenuCommand_ChromiumImpl(int context_index,              \
                                         ContextMenuCommand command_id); \
  void HandleAddTabToSuggestedGroupCommand(int context_index);           \
  void OnSuggestGroupForTabResult(                                       \
      std::vector<int> tab_indices, int context_index,                   \
      absl::StatusOr<tab_groups::TabGroupId> result);                    \
  void ExecuteContextMenuCommand

#define IsContextMenuCommandEnabled                            \
  IsContextMenuCommandEnabled_ChromiumImpl(                    \
      int context_index, ContextMenuCommand command_id) const; \
  bool IsContextMenuCommandEnabled

#define SelectRelativeTab(...)            \
  virtual SelectRelativeTab(__VA_ARGS__); \
  friend class BraveTabStripModel

// Added another closing selected tabs method to handle close activ tab
// in split tab. We only want to close active tab from split tab.
#define CloseSelectedTabs(...)    \
  CloseSelectedTabs(__VA_ARGS__); \
  virtual void CloseSelectedTabsWithSplitView()

#define DraggingTabsSession DraggingTabsSessionChromium
#define IsReadLaterSupportedForAny virtual IsReadLaterSupportedForAny
#define UpdateWebContentsStateAt virtual UpdateWebContentsStateAt

#include <chrome/browser/ui/tabs/tab_strip_model.h>  // IWYU pragma: export

#undef UpdateWebContentsStateAt
#undef IsReadLaterSupportedForAny
#undef DraggingTabsSession
#undef CloseSelectedTabs
#undef SelectRelativeTab
#undef IsContextMenuCommandEnabled
#undef ExecuteContextMenuCommand
#undef CommandCommerceProductSpecifications

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_TABS_TAB_STRIP_MODEL_H_
