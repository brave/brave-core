/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { OpenSettingsPage } from './open_settings_page.js'
import { GestureCommand } from './gesture_command.js'
import type {ActionRow, MouseActionRow} from './gesture_rows.js'

// Mirrors Chromium's real `IDC_OPTIONS`, which TS cannot import directly
// from the generated `chrome_command_ids.h`. Keep in sync with it.
export const IDC_OPTIONS = 40015

// Mirrors the JSON pref dict entry's wire format; the `command` key must
// match what `MaybeParseGestureActions` reads with `FindInt`. `value_param`
// is the same generic int-param key `int_param_util.h` reads via
// `kValueParamKey` for any bounded-enum action; which enum it is depends on
// `command` (see `hasIntParam`).
export interface ActionDict {
  command: number
  text_param?: string
  value_param?: number
}

function textParamFromDict(dict: ActionDict): string {
  return dict.text_param ?? ''
}

function openSettingsPageParamFromDict(dict: ActionDict): number {
  return dict.value_param ?? OpenSettingsPage.BASIC
}

// Whether `command` takes an `IntParamInfo` param, stored in `ActionRow`'s
// `openSettingsPage` field regardless of which bounded enum it actually
// holds (see `OpenSettingsPage`, `SmartGestureOpenInCommandParamType`,
// `SmartGestureSelectedTextSearchCommandParamType`). Keep in sync with the
// `Run*` handling in `gesture_action_dispatcher.cc`.
export function hasIntParam(command: number): boolean {
  return command === IDC_OPTIONS ||
    command === GestureCommand.OPEN_LINK_IN ||
    command === GestureCommand.OPEN_IMAGE_IN ||
    command === GestureCommand.SEARCH_SELECTED_TEXT
}

// A row with no URL/pattern and no Open Settings page, either because
// `command` takes no params or because no command is assigned yet.
export function defaultActionRow(command: number): ActionRow {
  return {
    command, textParam: '',
    openSettingsPage: OpenSettingsPage.BASIC
  }
}

// Converts a pref's stored dicts into a full `actionRows` (index 0
// primary), falling back to a single default primary action when empty.
export function actionRowsFromDicts(
  actions: ActionDict[] | undefined, defaultCommand: number): ActionRow[] {
  return actions?.length
    ? actions.map((entry) => ({
      command: entry.command,
      textParam: textParamFromDict(entry),
      openSettingsPage: openSettingsPageParamFromDict(entry),
    }))
    : [defaultActionRow(defaultCommand)]
}

// Builds a mouse rocker or mouse wheel row; see `MouseActionRow`.
export function buildActionRow(
  prefName: string, actions: ActionDict[] | undefined,
  defaultCommand: number): MouseActionRow {
  return {prefName, actionRows: actionRowsFromDicts(actions, defaultCommand)}
}

// Inserts after target if dragged started earlier, before if later, so
// dropping on the adjacent item is never a no-op.
export function moveAction(
  actionRows: ActionRow[], dragged: ActionRow,
  target: ActionRow): ActionRow[] {
  if (dragged === target) {
    return actionRows
  }
  const movingDown =
    actionRows.indexOf(dragged) < actionRows.indexOf(target)
  const withoutDragged = actionRows.filter((action) => action !== dragged)
  const targetIndex = withoutDragged.indexOf(target)
  const insertAt = movingDown ? targetIndex + 1 : targetIndex
  return [
    ...withoutDragged.slice(0, insertAt),
    dragged,
    ...withoutDragged.slice(insertAt),
  ]
}

// Applies `command` to the primary action, keeping any chained actions.
// Resets the primary action's params to `command`'s own defaults, since the
// previous command's `textParam`/`openSettingsPage` are meaningless for a
// different command.
export function applyCommandToActionRows(
  command: number, actionRows: ActionRow[] | undefined): ActionRow[] {
  const [, ...chainedActionRows] = actionRows ?? []
  return [defaultActionRow(command), ...chainedActionRows]
}

// Serializes to whichever wire key `command` actually dispatches with.
export function toActionDict(action: ActionRow): ActionDict {
  if (hasIntParam(action.command)) {
    return {
      command: action.command,
      value_param: action.openSettingsPage,
    }
  }
  return { command: action.command, text_param: action.textParam }
}

// Serializes a row's full action list (index 0 primary, the rest chained)
// into the ordered action list stored in the pref.
export function toActionDicts(actionRows: ActionRow[]): ActionDict[] {
  return actionRows.map(toActionDict)
}
