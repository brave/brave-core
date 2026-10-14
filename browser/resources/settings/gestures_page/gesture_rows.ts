/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// One action in a gesture's chain. Only one param is meaningful at a
// time, depending on `command`. `openSettingsPage` is the generic
// `IntParamInfo` slot for any command that takes one (see `hasIntParam` in
// `gesture_action_util.ts`), not just `IDC_OPTIONS`; which bounded enum it
// holds (`OpenSettingsPage`, `SmartGestureOpenInCommandParamType`,
// `SmartGestureSelectedTextSearchCommandParamType`) depends on `command`.
export interface ActionRow {
  command: number
  textParam: string
  openSettingsPage: number
}

export interface DisallowedSiteRow {
  origin: string
}

// A mouse rocker (left/right) or mouse wheel (up/down) action row; unlike
// other action rows, each is its own standalone list pref, named by
// `prefName`.
export interface MouseActionRow {
  prefName: string
  actionRows: ActionRow[]
}
