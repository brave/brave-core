/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {GestureCommand} from './gesture_command.js'
import type {ActionRow} from './gesture_rows.js'
import type {CustomGestureActionRow} from './custom_gesture_util.js'
import type {DirectionalGestureMappingActionRow} from './directional_gesture_mapping_util.js'

export function rowCommandSequence(row: { actionRows: ActionRow[] }): number[] {
  return row.actionRows.map((action) => action.command)
}

// Other gestures whose full command sequence matches `commands` exactly.
// `UNASSIGNED` never conflicts, since it is the shared default state.
export function findConflictingGestures(
  commands: number[],
  gestureTarget: { directionalGestureMappingKey?: string, customGestureId?: string },
  directionalGestureMappingActionRows: DirectionalGestureMappingActionRow[],
  customGestureActionRows: CustomGestureActionRow[]):
  { keys: string[], customGestureIds: string[] } {
  if (commands[0] === GestureCommand.UNASSIGNED) {
    return { keys: [], customGestureIds: [] }
  }
  const matches = (sequence: number[]) =>
    sequence.length === commands.length &&
    sequence.every((command, i) => command === commands[i])
  return {
    keys: directionalGestureMappingActionRows
      .filter((row) =>
        row.key !== gestureTarget.directionalGestureMappingKey &&
        matches(rowCommandSequence(row)))
      .map((row) => row.key),
    customGestureIds: customGestureActionRows
      .filter((row) =>
        row.id !== gestureTarget.customGestureId &&
        matches(rowCommandSequence(row)))
      .map((row) => row.id),
  }
}

// Names the conflicting gestures; custom gestures have no readable name,
// so those are just counted.
export function conflictDescription(
  keys: string[], customGestureIds: string[],
  i18n: (key: string, ...args: Array<string | number>) => string): string {
  if (!keys.length && !customGestureIds.length) {
    return ''
  }
  const parts = [...keys]
  if (customGestureIds.length === 1) {
    parts.push(i18n('braveGesturesConflictCustomGestureSingular'))
  } else if (customGestureIds.length > 1) {
    parts.push(i18n('braveGesturesConflictCustomGesturePlural', customGestureIds.length))
  }
  return i18n('braveGesturesCommandConflictSublabel', parts.join(', '))
}
