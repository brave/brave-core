/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {GestureCommand} from './gesture_command.js'
import {gestureKeyToIconNames} from './directional_gesture_icons.js'
import type {ActionRow} from './gesture_rows.js'
import type {ActionDict} from './gesture_action_util.js'
import {actionRowsFromDicts} from './gesture_action_util.js'

export const GESTURES_DIRECTIONAL_GESTURE_MAPPINGS_PREF =
  'brave.gestures.directional_gestures'

export interface DirectionalGestureMappingActionRow {
  key: string
  icons: string[]
  actionRows: ActionRow[]
}

// Keep in sync with `DefaultDirectionalGestureCommands` in
// `gesture_config_tracker.cc`.
const DEFAULT_DIRECTIONAL_GESTURE_MAPPINGS: Array<[string, number]> = [
  ['↓', 34014 /* IDC_NEW_TAB */],
  ['←', 33000 /* IDC_BACK */],
  ['→', 33001 /* IDC_FORWARD */],
  ['↑', GestureCommand.SCROLL_TO_TOP],
  ['↓→', 34015 /* IDC_CLOSE_TAB */],
  ['↓↑', 34000 /* IDC_NEW_WINDOW */],
  ['←↑', 34017 /* IDC_SELECT_PREVIOUS_TAB */],
  ['→↓', 34027 /* IDC_DUPLICATE_TAB */],
  ['→↑', 34016 /* IDC_SELECT_NEXT_TAB */],
  ['↑↓', 33002 /* IDC_RELOAD */],
  ['↑→', 34028 /* IDC_RESTORE_TAB */],
]

// Builds every mapping row from the pref's per-key overrides, falling back
// to each key's own default command when unset.
export function buildDirectionalGestureMappingActionRows(
  overrides: Record<string, ActionDict[]> | undefined):
  DirectionalGestureMappingActionRow[] {
  return DEFAULT_DIRECTIONAL_GESTURE_MAPPINGS.map(([key, defaultCommand]) => {
    const actions = overrides?.[key]
    return {
      key,
      icons: gestureKeyToIconNames(key),
      actionRows: actionRowsFromDicts(actions, defaultCommand),
    }
  })
}
