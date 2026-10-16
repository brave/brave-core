/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Mirrors `gesture_command_constants.h`; persisted in prefs, so never
// renumber. Any other value is a raw `IDC_*` id, not listed here.
export enum GestureCommand {
  UNASSIGNED = 0,
  NAVIGATE_TO_URL = 1,
  SCROLL_TO_TOP = 2,
  SWITCH_TO_TAB = 3,
}

// i18n keys for each sentinel's display name; any other command's name
// comes from the shared `CommandsCache` instead.
export const SPECIAL_COMMAND_I18N_KEYS: Record<GestureCommand, string> = {
  [GestureCommand.UNASSIGNED]: 'braveGesturesSpecialCommandUnassigned',
  [GestureCommand.NAVIGATE_TO_URL]: 'braveGesturesSpecialCommandNavigateToUrl',
  [GestureCommand.SCROLL_TO_TOP]: 'braveGesturesSpecialCommandScrollToTop',
  [GestureCommand.SWITCH_TO_TAB]: 'braveGesturesSpecialCommandSwitchToTab',
}
