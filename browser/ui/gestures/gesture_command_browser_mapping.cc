// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/gesture_command_browser_mapping.h"

#include "brave/app/brave_command_ids.h"
#include "chrome/app/chrome_command_ids.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

namespace gestures {

std::optional<int> MaybeToGestureCommandId(GestureCommandType gesture_command) {
  switch (gesture_command) {
    case GestureCommandType::kUnassigned:
      return std::nullopt;
    case GestureCommandType::kBack:
      return IDC_BACK;
    case GestureCommandType::kBookmarkPage:
      return IDC_BOOKMARK_THIS_TAB;
    case GestureCommandType::kClearBrowsingData:
      return IDC_CLEAR_BROWSING_DATA;
    case GestureCommandType::kCloseTab:
      return IDC_CLOSE_TAB;
    case GestureCommandType::kCloseWindow:
      return IDC_CLOSE_WINDOW;
    case GestureCommandType::kDuplicateTab:
      return IDC_DUPLICATE_TAB;
    case GestureCommandType::kEditSearchEngines:
      return IDC_EDIT_SEARCH_ENGINES;
    case GestureCommandType::kFind:
      return IDC_FIND;
    case GestureCommandType::kFocusAddressBar:
      return IDC_FOCUS_LOCATION;
    case GestureCommandType::kForward:
      return IDC_FORWARD;
    case GestureCommandType::kHardReload:
      return IDC_RELOAD_BYPASSING_CACHE;
    case GestureCommandType::kHome:
      return IDC_HOME;
    case GestureCommandType::kLastTab:
      return IDC_SELECT_LAST_TAB;
    case GestureCommandType::kManageExtensions:
      return IDC_MANAGE_EXTENSIONS;
    case GestureCommandType::kMuteTab:
      return IDC_WINDOW_MUTE_SITE;
    case GestureCommandType::kNavigateToUrl:
      // Navigating to a per-mapping URL isn't a fixed browser command; the
      // caller handles it directly instead of going through `IDC_*`.
      return std::nullopt;
    case GestureCommandType::kNewIncognitoWindow:
      return IDC_NEW_INCOGNITO_WINDOW;
    case GestureCommandType::kNewSplitTab:
      return IDC_NEW_SPLIT_TAB;
    case GestureCommandType::kNewTab:
      return IDC_NEW_TAB;
    case GestureCommandType::kNewWindow:
      return IDC_NEW_WINDOW;
    case GestureCommandType::kNextTab:
      return IDC_SELECT_NEXT_TAB;
    case GestureCommandType::kOpenDevTools:
      return IDC_DEV_TOOLS;
    case GestureCommandType::kOpenSettings:
      return IDC_OPTIONS;
    case GestureCommandType::kOpenTaskManager:
      return IDC_TASK_MANAGER;
    case GestureCommandType::kPreviousTab:
      return IDC_SELECT_PREVIOUS_TAB;
    case GestureCommandType::kPrint:
      return IDC_PRINT;
    case GestureCommandType::kReload:
      return IDC_RELOAD;
    case GestureCommandType::kReopenClosedTab:
      return IDC_RESTORE_TAB;
    case GestureCommandType::kSavePage:
      return IDC_SAVE_PAGE;
    case GestureCommandType::kScrollToTop:
      // Scrolls the page content, not a browser command; the caller handles
      // it directly instead of going through `IDC_*`.
      return std::nullopt;
    case GestureCommandType::kShowAbout:
      return IDC_ABOUT;
    case GestureCommandType::kShowBookmarkManager:
      return IDC_SHOW_BOOKMARK_MANAGER;
    case GestureCommandType::kShowDownloads:
      return IDC_SHOW_DOWNLOADS;
    case GestureCommandType::kShowHistory:
      return IDC_SHOW_HISTORY;
    case GestureCommandType::kStop:
      return IDC_STOP;
    case GestureCommandType::kSwitchToTab:
      // Switches to a tab found by matching a per-mapping glob pattern; not
      // a fixed browser command, so the caller handles it directly instead
      // of going through `IDC_*`.
      return std::nullopt;
    case GestureCommandType::kToggleBookmarkBar:
      return IDC_SHOW_BOOKMARK_BAR;
    case GestureCommandType::kToggleFullscreen:
      return IDC_FULLSCREEN;
    case GestureCommandType::kToggleLeo:
      return IDC_TOGGLE_AI_CHAT;
    case GestureCommandType::kToggleShields:
      return IDC_TOGGLE_SHIELDS;
    case GestureCommandType::kToggleSidebar:
      return IDC_TOGGLE_SIDEBAR;
    case GestureCommandType::kToggleVerticalTabs:
      return IDC_TOGGLE_VERTICAL_TABS;
    case GestureCommandType::kToggleVerticalTabsExpanded:
      return IDC_TOGGLE_VERTICAL_TABS_EXPANDED;
    case GestureCommandType::kViewPasswords:
      return IDC_VIEW_PASSWORDS;
    case GestureCommandType::kViewSource:
      return IDC_VIEW_SOURCE;
    case GestureCommandType::kZoomIn:
      return IDC_ZOOM_PLUS;
    case GestureCommandType::kZoomOut:
      return IDC_ZOOM_MINUS;
    case GestureCommandType::kZoomReset:
      return IDC_ZOOM_NORMAL;
  }
}

std::u16string ToDisplayName(GestureCommandType gesture_command) {
  switch (gesture_command) {
    case GestureCommandType::kUnassigned:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_UNASSIGNED);
    case GestureCommandType::kBack:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_BACK);
    case GestureCommandType::kBookmarkPage:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_BOOKMARK_PAGE);
    case GestureCommandType::kClearBrowsingData:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_CLEAR_BROWSING_DATA);
    case GestureCommandType::kCloseTab:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_CLOSE_TAB);
    case GestureCommandType::kCloseWindow:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_CLOSE_WINDOW);
    case GestureCommandType::kDuplicateTab:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_DUPLICATE_TAB);
    case GestureCommandType::kEditSearchEngines:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_EDIT_SEARCH_ENGINES);
    case GestureCommandType::kFind:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_FIND);
    case GestureCommandType::kFocusAddressBar:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_FOCUS_ADDRESS_BAR);
    case GestureCommandType::kForward:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_FORWARD);
    case GestureCommandType::kHardReload:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_HARD_RELOAD);
    case GestureCommandType::kHome:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_HOME);
    case GestureCommandType::kLastTab:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_LAST_TAB);
    case GestureCommandType::kManageExtensions:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_MANAGE_EXTENSIONS);
    case GestureCommandType::kMuteTab:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_MUTE_TAB);
    case GestureCommandType::kNavigateToUrl:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_NAVIGATE_TO_URL);
    case GestureCommandType::kNewIncognitoWindow:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_NEW_INCOGNITO_WINDOW);
    case GestureCommandType::kNewSplitTab:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_NEW_SPLIT_TAB);
    case GestureCommandType::kNewTab:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_NEW_TAB);
    case GestureCommandType::kNewWindow:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_NEW_WINDOW);
    case GestureCommandType::kNextTab:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_NEXT_TAB);
    case GestureCommandType::kOpenDevTools:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_OPEN_DEV_TOOLS);
    case GestureCommandType::kOpenSettings:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_OPEN_SETTINGS);
    case GestureCommandType::kOpenTaskManager:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_OPEN_TASK_MANAGER);
    case GestureCommandType::kPreviousTab:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_PREVIOUS_TAB);
    case GestureCommandType::kPrint:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_PRINT);
    case GestureCommandType::kReload:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_RELOAD);
    case GestureCommandType::kReopenClosedTab:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_REOPEN_CLOSED_TAB);
    case GestureCommandType::kSavePage:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_SAVE_PAGE);
    case GestureCommandType::kScrollToTop:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_SCROLL_TO_TOP);
    case GestureCommandType::kShowAbout:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_SHOW_ABOUT);
    case GestureCommandType::kShowBookmarkManager:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_SHOW_BOOKMARK_MANAGER);
    case GestureCommandType::kShowDownloads:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_SHOW_DOWNLOADS);
    case GestureCommandType::kShowHistory:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_SHOW_HISTORY);
    case GestureCommandType::kStop:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_STOP);
    case GestureCommandType::kSwitchToTab:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_SWITCH_TO_TAB);
    case GestureCommandType::kToggleBookmarkBar:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_TOGGLE_BOOKMARK_BAR);
    case GestureCommandType::kToggleFullscreen:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_TOGGLE_FULLSCREEN);
    case GestureCommandType::kToggleLeo:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_TOGGLE_LEO);
    case GestureCommandType::kToggleShields:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_TOGGLE_SHIELDS);
    case GestureCommandType::kToggleSidebar:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_TOGGLE_SIDEBAR);
    case GestureCommandType::kToggleVerticalTabs:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_TOGGLE_VERTICAL_TABS);
    case GestureCommandType::kToggleVerticalTabsExpanded:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_TOGGLE_VERTICAL_TABS_EXPANDED);
    case GestureCommandType::kViewPasswords:
      return l10n_util::GetStringUTF16(
          IDS_BRAVE_GESTURES_COMMAND_VIEW_PASSWORDS);
    case GestureCommandType::kViewSource:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_VIEW_SOURCE);
    case GestureCommandType::kZoomIn:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_ZOOM_IN);
    case GestureCommandType::kZoomOut:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_ZOOM_OUT);
    case GestureCommandType::kZoomReset:
      return l10n_util::GetStringUTF16(IDS_BRAVE_GESTURES_COMMAND_ZOOM_RESET);
  }
}

}  // namespace gestures
