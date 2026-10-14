/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Keep in sync with GestureCommand in
// brave/components/gestures/core/common/gesture_command_types.h.
// These values are persisted directly in prefs. Never renumber or reorder
// existing entries; only append new commands at the end of the list, and
// keep both enums in sync.
export enum GestureCommand {
  UNASSIGNED = 0,
  BACK = 1,
  BOOKMARK_PAGE = 2,
  CLEAR_BROWSING_DATA = 3,
  CLOSE_TAB = 4,
  CLOSE_WINDOW = 5,
  DUPLICATE_TAB = 6,
  EDIT_SEARCH_ENGINES = 7,
  FIND = 8,
  FOCUS_ADDRESS_BAR = 9,
  FORWARD = 10,
  HARD_RELOAD = 11,
  HOME = 12,
  LAST_TAB = 13,
  MANAGE_EXTENSIONS = 14,
  MUTE_TAB = 15,
  NAVIGATE_TO_URL = 16,
  NEW_INCOGNITO_WINDOW = 17,
  NEW_SPLIT_TAB = 18,
  NEW_TAB = 19,
  NEW_WINDOW = 20,
  NEXT_TAB = 21,
  OPEN_DEV_TOOLS = 22,
  OPEN_SETTINGS = 23,
  OPEN_TASK_MANAGER = 24,
  PREVIOUS_TAB = 25,
  PRINT = 26,
  RELOAD = 27,
  REOPEN_CLOSED_TAB = 28,
  SAVE_PAGE = 29,
  SCROLL_TO_TOP = 30,
  SHOW_ABOUT = 31,
  SHOW_BOOKMARK_MANAGER = 32,
  SHOW_DOWNLOADS = 33,
  SHOW_HISTORY = 34,
  STOP = 35,
  TOGGLE_BOOKMARK_BAR = 36,
  TOGGLE_FULLSCREEN = 37,
  TOGGLE_LEO = 38,
  TOGGLE_SHIELDS = 39,
  TOGGLE_SIDEBAR = 40,
  TOGGLE_VERTICAL_TABS = 41,
  TOGGLE_VERTICAL_TABS_EXPANDED = 42,
  VIEW_PASSWORDS = 43,
  VIEW_SOURCE = 44,
  ZOOM_IN = 45,
  ZOOM_OUT = 46,
  ZOOM_RESET = 47,
  SWITCH_TO_TAB = 48,
}

// Order of the flat command dropdown list. `UNASSIGNED` stays first since
// it is the default, unbound state; the rest are alphabetical by display
// name (see COMMAND_I18N_KEYS), not by enum value.
export const COMMAND_ORDER: GestureCommand[] = [
  GestureCommand.UNASSIGNED,
  GestureCommand.SHOW_ABOUT,  // "About Brave"
  GestureCommand.BACK,
  GestureCommand.BOOKMARK_PAGE,
  GestureCommand.CLEAR_BROWSING_DATA,
  GestureCommand.CLOSE_TAB,
  GestureCommand.CLOSE_WINDOW,
  GestureCommand.SHOW_DOWNLOADS,  // "Downloads"
  GestureCommand.DUPLICATE_TAB,
  GestureCommand.EDIT_SEARCH_ENGINES,
  GestureCommand.TOGGLE_VERTICAL_TABS_EXPANDED,  // "Expand or collapse..."
  GestureCommand.FIND,  // "Find in page"
  GestureCommand.FOCUS_ADDRESS_BAR,
  GestureCommand.FORWARD,
  GestureCommand.HARD_RELOAD,
  GestureCommand.SHOW_HISTORY,  // "History"
  GestureCommand.HOME,
  GestureCommand.MANAGE_EXTENSIONS,
  GestureCommand.MUTE_TAB,  // "Mute site"
  GestureCommand.NAVIGATE_TO_URL,
  GestureCommand.NEW_INCOGNITO_WINDOW,  // "New private window"
  GestureCommand.NEW_SPLIT_TAB,
  GestureCommand.NEW_TAB,
  GestureCommand.NEW_WINDOW,
  GestureCommand.NEXT_TAB,
  GestureCommand.SHOW_BOOKMARK_MANAGER,  // "Open bookmark manager"
  GestureCommand.OPEN_DEV_TOOLS,  // "Open developer tools"
  GestureCommand.OPEN_SETTINGS,
  GestureCommand.OPEN_TASK_MANAGER,
  GestureCommand.PREVIOUS_TAB,
  GestureCommand.PRINT,
  GestureCommand.RELOAD,
  GestureCommand.REOPEN_CLOSED_TAB,
  GestureCommand.ZOOM_RESET,  // "Reset zoom"
  GestureCommand.SAVE_PAGE,
  GestureCommand.SCROLL_TO_TOP,
  GestureCommand.STOP,
  GestureCommand.LAST_TAB,  // "Switch to last tab"
  GestureCommand.SWITCH_TO_TAB,
  GestureCommand.TOGGLE_BOOKMARK_BAR,
  GestureCommand.TOGGLE_FULLSCREEN,
  GestureCommand.TOGGLE_LEO,
  GestureCommand.TOGGLE_SHIELDS,
  GestureCommand.TOGGLE_SIDEBAR,
  GestureCommand.TOGGLE_VERTICAL_TABS,
  GestureCommand.VIEW_PASSWORDS,
  GestureCommand.VIEW_SOURCE,
  GestureCommand.ZOOM_IN,
  GestureCommand.ZOOM_OUT,
]

// Keep in sync with DefaultMappings in
// brave/components/gestures/core/common/default_gesture_mappings.cc
export const DEFAULT_GESTURE_MAPPINGS: Array<[string, GestureCommand]> = [
  ['↓', GestureCommand.NEW_TAB],
  ['←', GestureCommand.BACK],
  ['→', GestureCommand.FORWARD],
  ['↑', GestureCommand.SCROLL_TO_TOP],
  ['↓→', GestureCommand.CLOSE_TAB],
  ['↓↑', GestureCommand.NEW_WINDOW],
  ['←↑', GestureCommand.PREVIOUS_TAB],
  ['→↓', GestureCommand.DUPLICATE_TAB],
  ['→↑', GestureCommand.NEXT_TAB],
  ['↑↓', GestureCommand.RELOAD],
  ['↑→', GestureCommand.REOPEN_CLOSED_TAB],
]

// Leo icon names (see @brave/leo/icons/meta.d.ts), not raw glyphs. Keep in
// sync with DirectionToKeyCode in
// brave/components/gestures/core/common/default_gesture_mappings.cc.
const GESTURE_KEY_ICON_NAMES: Record<string, string> = {
  '↑': 'arrow-up', '↓': 'arrow-down', '←': 'arrow-left', '→': 'arrow-right',
  '↖': 'arrow-diagonal-up-left', '↗': 'arrow-diagonal-up-right',
  '↙': 'arrow-diagonal-down-left', '↘': 'arrow-diagonal-down-right',
}

// i18n keys for each command's display name (registered in
// `brave_settings_localized_strings_provider.cc`, reusing the same
// `IDS_BRAVE_GESTURES_COMMAND_*` strings shown in the trail overlay by
// `GestureController::ToDisplayName`).
export const COMMAND_I18N_KEYS: Record<GestureCommand, string> = {
  [GestureCommand.UNASSIGNED]: 'braveGesturesCommandUnassigned',
  [GestureCommand.BACK]: 'braveGesturesCommandBack',
  [GestureCommand.FORWARD]: 'braveGesturesCommandForward',
  [GestureCommand.NEW_TAB]: 'braveGesturesCommandNewTab',
  [GestureCommand.CLOSE_TAB]: 'braveGesturesCommandCloseTab',
  [GestureCommand.RELOAD]: 'braveGesturesCommandReload',
  [GestureCommand.REOPEN_CLOSED_TAB]:
    'braveGesturesCommandReopenClosedTab',
  [GestureCommand.PREVIOUS_TAB]: 'braveGesturesCommandPreviousTab',
  [GestureCommand.NEXT_TAB]: 'braveGesturesCommandNextTab',
  [GestureCommand.NEW_WINDOW]: 'braveGesturesCommandNewWindow',
  [GestureCommand.DUPLICATE_TAB]:
    'braveGesturesCommandDuplicateTab',
  [GestureCommand.NEW_INCOGNITO_WINDOW]:
    'braveGesturesCommandNewIncognitoWindow',
  [GestureCommand.CLOSE_WINDOW]: 'braveGesturesCommandCloseWindow',
  [GestureCommand.ZOOM_IN]: 'braveGesturesCommandZoomIn',
  [GestureCommand.ZOOM_OUT]: 'braveGesturesCommandZoomOut',
  [GestureCommand.ZOOM_RESET]: 'braveGesturesCommandZoomReset',
  [GestureCommand.TOGGLE_FULLSCREEN]:
    'braveGesturesCommandToggleFullscreen',
  [GestureCommand.FIND]: 'braveGesturesCommandFind',
  [GestureCommand.PRINT]: 'braveGesturesCommandPrint',
  [GestureCommand.BOOKMARK_PAGE]: 'braveGesturesCommandBookmarkPage',
  [GestureCommand.MUTE_TAB]: 'braveGesturesCommandMuteTab',
  [GestureCommand.SHOW_DOWNLOADS]:
    'braveGesturesCommandShowDownloads',
  [GestureCommand.SHOW_HISTORY]: 'braveGesturesCommandShowHistory',
  [GestureCommand.STOP]: 'braveGesturesCommandStop',
  [GestureCommand.HOME]: 'braveGesturesCommandHome',
  [GestureCommand.HARD_RELOAD]: 'braveGesturesCommandHardReload',
  [GestureCommand.VIEW_SOURCE]: 'braveGesturesCommandViewSource',
  [GestureCommand.FOCUS_ADDRESS_BAR]:
    'braveGesturesCommandFocusAddressBar',
  [GestureCommand.OPEN_DEV_TOOLS]:
    'braveGesturesCommandOpenDevTools',
  [GestureCommand.TOGGLE_BOOKMARK_BAR]:
    'braveGesturesCommandToggleBookmarkBar',
  [GestureCommand.OPEN_SETTINGS]: 'braveGesturesCommandOpenSettings',
  [GestureCommand.SAVE_PAGE]: 'braveGesturesCommandSavePage',
  [GestureCommand.LAST_TAB]: 'braveGesturesCommandLastTab',
  [GestureCommand.SHOW_BOOKMARK_MANAGER]:
    'braveGesturesCommandShowBookmarkManager',
  [GestureCommand.OPEN_TASK_MANAGER]: 'braveGesturesCommandOpenTaskManager',
  [GestureCommand.MANAGE_EXTENSIONS]: 'braveGesturesCommandManageExtensions',
  [GestureCommand.CLEAR_BROWSING_DATA]:
    'braveGesturesCommandClearBrowsingData',
  [GestureCommand.SHOW_ABOUT]: 'braveGesturesCommandShowAbout',
  [GestureCommand.SCROLL_TO_TOP]: 'braveGesturesCommandScrollToTop',
  [GestureCommand.EDIT_SEARCH_ENGINES]:
    'braveGesturesCommandEditSearchEngines',
  [GestureCommand.VIEW_PASSWORDS]: 'braveGesturesCommandViewPasswords',
  [GestureCommand.TOGGLE_SHIELDS]: 'braveGesturesCommandToggleShields',
  [GestureCommand.TOGGLE_LEO]: 'braveGesturesCommandToggleLeo',
  [GestureCommand.TOGGLE_SIDEBAR]: 'braveGesturesCommandToggleSidebar',
  [GestureCommand.TOGGLE_VERTICAL_TABS]:
    'braveGesturesCommandToggleVerticalTabs',
  [GestureCommand.TOGGLE_VERTICAL_TABS_EXPANDED]:
    'braveGesturesCommandToggleVerticalTabsExpanded',
  [GestureCommand.NEW_SPLIT_TAB]: 'braveGesturesCommandNewSplitTab',
  [GestureCommand.NAVIGATE_TO_URL]: 'braveGesturesCommandNavigateToUrl',
  [GestureCommand.SWITCH_TO_TAB]: 'braveGesturesCommandSwitchToTab',
}

export function gestureKeyToIconNames(key: string): string[] {
  return key.split('').map((c) => GESTURE_KEY_ICON_NAMES[c] ?? c)
}

// `GestureController::RunGestureAction` only navigates for a URL with an
// http/https scheme; a bare domain like "example.com" has no scheme and is
// otherwise silently dropped. Assumes https, matching address bar behavior
// for a scheme-less entry.
export function withUrlScheme(url: string): string {
  const lowercase = url.toLowerCase()
  const hasScheme =
    lowercase.startsWith('http://') || lowercase.startsWith('https://')
  return url && !hasScheme ? `https://${url}` : url
}
