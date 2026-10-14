// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_TYPES_H_
#define BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_TYPES_H_

namespace gestures {

// Commands a gesture can trigger. Deliberately independent of the browser's
// `IDC_*` command ids, which this component must not depend on. Translation to
// an `IDC_*` value happens in the `browser/` layer that actually dispatches the
// command.
//
// The underlying values are persisted directly in prefs and in the mirrored
// `GestureCommand` TypeScript enum in `gesture_command.ts`. Never renumber or
// reorder existing entries; only append new commands at the end of the list,
// and keep both enums in sync.
enum class GestureCommandType {
  kUnassigned = 0,
  kBack = 1,
  kBookmarkPage = 2,
  kClearBrowsingData = 3,
  kCloseTab = 4,
  kCloseWindow = 5,
  kDuplicateTab = 6,
  kEditSearchEngines = 7,
  kFind = 8,
  kFocusAddressBar = 9,
  kForward = 10,
  kHardReload = 11,
  kHome = 12,
  kLastTab = 13,
  kManageExtensions = 14,
  kMuteTab = 15,
  kNavigateToUrl = 16,
  kNewIncognitoWindow = 17,
  kNewSplitTab = 18,
  kNewTab = 19,
  kNewWindow = 20,
  kNextTab = 21,
  kOpenDevTools = 22,
  kOpenSettings = 23,
  kOpenTaskManager = 24,
  kPreviousTab = 25,
  kPrint = 26,
  kReload = 27,
  kReopenClosedTab = 28,
  kSavePage = 29,
  kScrollToTop = 30,
  kShowAbout = 31,
  kShowBookmarkManager = 32,
  kShowDownloads = 33,
  kShowHistory = 34,
  kStop = 35,
  kToggleBookmarkBar = 36,
  kToggleFullscreen = 37,
  kToggleLeo = 38,
  kToggleShields = 39,
  kToggleSidebar = 40,
  kToggleVerticalTabs = 41,
  kToggleVerticalTabsExpanded = 42,
  kViewPasswords = 43,
  kViewSource = 44,
  kZoomIn = 45,
  kZoomOut = 46,
  kZoomReset = 47,
  kSwitchToTab = 48,

  kMaxValue = kSwitchToTab,
};

}  // namespace gestures

#endif  // BRAVE_COMPONENTS_GESTURES_CORE_COMMON_ACTIONS_GESTURE_COMMAND_TYPES_H_
