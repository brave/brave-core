/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {GestureCommand} from './gesture_command.js'
import {toActionDict} from './gesture_action_util.js'
import type {
  ActionRow, MouseRockerActionRow, MouseWheelActionRow
} from './gesture_rows.js'
import type {CustomGestureActionRow} from './custom_gesture_util.js'
import type {DirectionalGestureMappingActionRow} from './directional_gesture_mapping_util.js'
import type {SmartGestureMappingActionRow} from './smart_gesture_mapping_util.js'
import {smartGestureKeyLabel} from './smart_gesture_mapping_util.js'

// Two actions with the same `command` but different params (e.g. two
// "Open Settings Page" actions targeting different pages, or two
// "Navigate to URL" actions with different URLs) do different things and
// must not be treated as the same action; `toActionDict` already discards
// whichever param isn't meaningful for `command`, so comparing its
// serialization captures exactly the params that matter.
function actionSignature(action: ActionRow): string {
  return JSON.stringify(toActionDict(action))
}

export function rowCommandSequence(row: { actionRows: ActionRow[] }): string[] {
  return row.actionRows.map(actionSignature)
}

export interface GestureConflicts {
  // Directional gestures have a readable name (the arrow key, e.g. "↓"), so
  // those are named individually.
  keys: string[]
  // Smart gestures are also named individually (e.g. "Link →"), via
  // `smartGestureKeyLabel`.
  smartGestureKeys: string[]
  // Custom gestures, mouse rocker gestures, and mouse wheel gestures have
  // no readable per-row name, so those are just counted.
  customGestureCount: number
  mouseRockerCount: number
  mouseWheelCount: number
}

// Other gestures, of any kind, whose full action sequence matches
// `actionRows` exactly. `UNASSIGNED` never conflicts, since it is the
// shared default state.
export function findConflictingGestures(
  actionRows: ActionRow[],
  gestureTarget: {
    directionalGestureMappingKey?: string, customGestureId?: string,
    smartGestureKey?: string,
    mouseRockerPrefName?: string, mouseWheelPrefName?: string
  },
  directionalGestureMappingActionRows: DirectionalGestureMappingActionRow[],
  customGestureActionRows: CustomGestureActionRow[],
  smartGestureMappingActionRows: SmartGestureMappingActionRow[],
  mouseRockerActionRows: MouseRockerActionRow[],
  mouseWheelActionRows: MouseWheelActionRow[]): GestureConflicts {
  if (actionRows[0]?.command === GestureCommand.UNASSIGNED) {
    return {
      keys: [], smartGestureKeys: [], customGestureCount: 0,
      mouseRockerCount: 0, mouseWheelCount: 0
    }
  }
  const commands = actionRows.map(actionSignature)
  const matches = (row: { actionRows: ActionRow[] }) => {
    const sequence = rowCommandSequence(row)
    return sequence.length === commands.length &&
      sequence.every((command, i) => command === commands[i])
  }
  // Mouse rocker and mouse wheel gestures fire from a genuinely different
  // input mechanism (a chord click, a scroll) than a drawn gesture, so
  // sharing a command with one of those is not a real conflict; only count
  // conflicts against gestures of the same kind for those two. Directional,
  // custom, and smart gestures, however, are all read from the exact same
  // drawn mouse path (see `ResolveGestureMatch`/`MaybeFinish`), competing to
  // explain the same physical gesture (a smart gesture even takes priority
  // over the other two when it has captured content), so any one of the
  // three conflicts with the other two, and with itself.
  const isDrawnGestureTarget =
    gestureTarget.directionalGestureMappingKey !== undefined ||
    gestureTarget.customGestureId !== undefined ||
    gestureTarget.smartGestureKey !== undefined
  return {
    keys: !isDrawnGestureTarget ? []
      : directionalGestureMappingActionRows
        .filter((row) =>
          row.key !== gestureTarget.directionalGestureMappingKey && matches(row))
        .map((row) => row.key),
    smartGestureKeys: !isDrawnGestureTarget ? []
      : smartGestureMappingActionRows
        .filter((row) =>
          row.key !== gestureTarget.smartGestureKey && matches(row))
        .map((row) => smartGestureKeyLabel(row.key)),
    customGestureCount: !isDrawnGestureTarget ? 0
      : customGestureActionRows
        .filter((row) =>
          row.id !== gestureTarget.customGestureId && matches(row)).length,
    mouseRockerCount: gestureTarget.mouseRockerPrefName === undefined ? 0
      : mouseRockerActionRows
        .filter((row) =>
          row.prefName !== gestureTarget.mouseRockerPrefName && matches(row))
        .length,
    mouseWheelCount: gestureTarget.mouseWheelPrefName === undefined ? 0
      : mouseWheelActionRows
        .filter((row) =>
          row.prefName !== gestureTarget.mouseWheelPrefName && matches(row))
        .length,
  }
}

function pushCountedConflict(
  parts: string[], count: number, singularKey: string, pluralKey: string,
  i18n: (key: string, ...args: Array<string | number>) => string) {
  if (count === 1) {
    parts.push(i18n(singularKey))
  } else if (count > 1) {
    parts.push(i18n(pluralKey, count))
  }
}

// Names the conflicting gestures.
export function conflictDescription(
  conflicts: GestureConflicts,
  i18n: (key: string, ...args: Array<string | number>) => string): string {
  const parts = [...conflicts.keys, ...conflicts.smartGestureKeys]
  pushCountedConflict(
    parts, conflicts.customGestureCount,
    'braveGesturesConflictCustomGestureSingular',
    'braveGesturesConflictCustomGesturePlural', i18n)
  pushCountedConflict(
    parts, conflicts.mouseRockerCount,
    'braveGesturesConflictMouseRockerSingular',
    'braveGesturesConflictMouseRockerPlural', i18n)
  pushCountedConflict(
    parts, conflicts.mouseWheelCount,
    'braveGesturesConflictMouseWheelSingular',
    'braveGesturesConflictMouseWheelPlural', i18n)
  if (!parts.length) {
    return ''
  }
  // "←, ↓→, a custom gesture" reads as a sentence fragment, not a proper
  // list; `Intl.ListFormat` joins the final item with "and" instead, per the
  // page's own locale. `|| undefined` falls back to the runtime default
  // locale instead of passing `Intl.ListFormat` an empty string, which it
  // throws a `RangeError` on, if `<html lang>` isn't set on this page.
  const formattedList = new Intl.ListFormat(
    document.documentElement.lang || undefined,
    { style: 'long', type: 'conjunction' })
    .format(parts)
  return i18n('braveGesturesCommandConflictSublabel', formattedList)
}
