/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/tabs/tab_strip_model.h"

#include "chrome/browser/ui/views/tabs/dragging/tab_drag_controller.h"
#include "components/tabs/public/tab_strip_collection.h"

// To avoid enumeration values not handled in switch error.
#define CommandAddNote                  \
  CommandAddNote:                       \
  case CommandRestoreTab:               \
  case CommandBookmarkAllTabs:          \
  case CommandShowVerticalTabs:         \
  case CommandToggleTabMuted:           \
  case CommandBringAllTabsToThisWindow: \
  case CommandCloseDuplicateTabs:       \
  case CommandOpenInContainer:          \
  case CommandRenameTab

#define DraggingTabsSession DraggingTabsSessionChromium

// Pass additional parameter for contents_data_->AddTabRecursive().
#define AddTabRecursive(...)                                    \
  /* meaningless call to address "contents_data_->" */          \
  pinned_collection();                                          \
  /* passing nullptr as opener when creating a empty new tab */ \
  /* in order not to create a nested tree node */               \
  auto* opener = tab_model->opener_was_set_for_empty_new_tab()  \
                     ? nullptr                                  \
                     : tab_model->opener();                     \
  contents_data_->AddTabRecursive(__VA_ARGS__, opener)

// When adding a new tab with a "TYPED" transition, set the
// opener_was_set_for_empty_new_tab() flag to true to avoid creating a nested
// tree node. This is followed by inherit_opener = true; in the original code.
// The current condition is:
// * Page transition type includes ui::PAGE_TRANSITION_TYPED qualifier
// * The index is the last index in the tab strip (count())
#define BRAVE_TAB_STRIP_MODEL_ADD_TAB \
  tab->set_opener_was_set_for_empty_new_tab();

#include <chrome/browser/ui/tabs/tab_strip_model.cc>  // IWYU pragma: export

#undef BRAVE_TAB_STRIP_MODEL_ADD_TAB
#undef AddTabRecursive
#undef DraggingTabsSession
#undef CommandAddNote
