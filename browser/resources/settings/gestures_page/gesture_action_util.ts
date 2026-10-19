/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { OpenSettingsPage } from './open_settings_page.js'
import type {ActionRow} from './gesture_rows.js'

// Mirrors Chromium's real `IDC_OPTIONS`, which TS cannot import directly
// from the generated `chrome_command_ids.h`. Keep in sync with it.
export const IDC_OPTIONS = 40015

// Mirrors the JSON pref dict entry's wire format; the `command` key must
// match what `MaybeParseGestureActions` reads with `FindInt`.
export interface ActionDict {
  command: number
  text_param?: string
  open_settings_page?: OpenSettingsPage
}

function textParamFromDict(dict: ActionDict): string {
  return dict.text_param ?? ''
}

// Open Settings' target page has its own dedicated wire key, since it is
// a bounded enum rather than free text.
function openSettingsPageParamFromDict(dict: ActionDict): OpenSettingsPage {
  return dict.open_settings_page ?? OpenSettingsPage.BASIC
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
export function applyCommandToActionRows(
  command: number, actionRows: ActionRow[] | undefined): ActionRow[] {
  const [primaryActionRow, ...chainedActionRows] =
    actionRows ?? [defaultActionRow(command)]
  return [{ ...primaryActionRow, command }, ...chainedActionRows]
}

// Serializes to whichever wire key `command` actually dispatches with.
function toActionDict(action: ActionRow): ActionDict {
  if (action.command === IDC_OPTIONS) {
    return {
      command: action.command,
      open_settings_page: action.openSettingsPage,
    }
  }
  return { command: action.command, text_param: action.textParam }
}

// Serializes a row's full action list (index 0 primary, the rest chained)
// into the ordered action list stored in the pref.
export function toActionDicts(actionRows: ActionRow[]): ActionDict[] {
  return actionRows.map(toActionDict)
}
