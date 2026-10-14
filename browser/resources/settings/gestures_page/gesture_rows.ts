/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {GestureCommand} from './gesture_command.js'
import type {Point} from './custom_gesture_geometry.js'

export interface ChainedActionRow {
  gestureCommand: GestureCommand
  navigateToUrl: string
}

// Mirrors the JSON pref dict entry's wire format; the `command` key must
// match what `MaybeParseGestureActions` reads with `FindInt`.
export interface ChainedActionEntry {
  command: GestureCommand
  navigate_to_url?: string
  switch_to_tab_pattern?: string
}

// `navigateToUrl` on the row types below is shared storage for either a
// Navigate to URL destination or a Switch to Tab glob pattern, since a
// given action's `gestureCommand` is only ever one of the two; which wire
// key it serializes to and parses from depends on that command, kept in
// sync here and in the parsing/dialog code in gestures_page.ts.
export function textParamFromEntry(entry: ChainedActionEntry): string {
  return entry.navigate_to_url ?? entry.switch_to_tab_pattern ?? ''
}

export interface MappingRow {
  key: string
  icons: string[]
  gestureCommand: GestureCommand
  navigateToUrl: string
  chainedActions: ChainedActionRow[]
}

export interface CommandOption {
  name: string
  value: GestureCommand
}

export interface DisallowedSiteRow {
  origin: string
}

// A rocker/wheel action row (rock left/right, wheel up/down). Unlike
// `MappingRow`/`CustomGestureRow`, which are entries in a dict/array pref
// keyed by a gesture key or id, each of these is its own standalone list
// pref holding exactly this row's chain, so `prefName` identifies which pref
// to persist back to instead of a key/id.
export interface RockerWheelRow {
  prefName: string
  gestureCommand: GestureCommand
  navigateToUrl: string
  chainedActions: ChainedActionRow[]
}

export interface CustomGestureRow {
  id: string
  gestureCommand: GestureCommand
  previewPath: string
  navigateToUrl: string
  chainedActions: ChainedActionRow[]
}

export type CustomGesturePoint = Point

export interface CustomGestureEntry {
  id: string
  points: CustomGesturePoint[]
  actions?: ChainedActionEntry[]
}

// Serializes `navigateToUrl`'s shared storage (see its own comment) to
// whichever wire key `gestureCommand` actually dispatches with.
function toActionEntry(
    action: { gestureCommand: GestureCommand, navigateToUrl: string }):
    ChainedActionEntry {
  if (action.gestureCommand === GestureCommand.SWITCH_TO_TAB) {
    return {
      command: action.gestureCommand,
      switch_to_tab_pattern: action.navigateToUrl,
    }
  }
  return { command: action.gestureCommand, navigate_to_url: action.navigateToUrl }
}

// Combines a row's primary command/URL with its chained actions into the
// full ordered action list the backend persists.
export function buildActionEntries(
  primary: { gestureCommand: GestureCommand, navigateToUrl: string },
  chainedActions: ChainedActionRow[]): ChainedActionEntry[] {
  return [toActionEntry(primary), ...chainedActions.map(toActionEntry)]
}
