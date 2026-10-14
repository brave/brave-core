/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {GestureCommand} from './gesture_command.js'
import {SmartGestureOpenInCommandParamType} from
  './smart_gesture_open_in_command_param.js'
import {SmartGestureSelectedTextSearchCommandParamType} from
  './smart_gesture_selected_text_search_command_param.js'
import type {ActionRow} from './gesture_rows.js'
import type {ActionDict} from './gesture_action_util.js'
import {actionRowsFromDicts} from './gesture_action_util.js'

export const GESTURES_SMART_GESTURES_PREF = 'brave.gestures.smart_gestures'

// Keep in sync with `ContentTypeKey` in `smart_gesture_key_util.cc`.
export enum SmartGestureContentType {
  LINK = 'link',
  SELECTED_TEXT = 'selected_text',
  IMAGE = 'image',
}

// Keep in sync with `DirectionTypeKey` in `smart_gesture_key_util.cc`.
const DIRECTIONS = ['up', 'down', 'left', 'right']

// Keep in sync with `gestureKeyToIconNames`'s arrow entries in
// `directional_gesture_icons.ts`.
const DIRECTION_ICON_NAMES: Record<string, string> = {
  up: 'arrow-up', down: 'arrow-down', left: 'arrow-left', right: 'arrow-right',
}

const DIRECTION_ARROWS: Record<string, string> = {
  up: '↑', down: '↓', left: '←', right: '→',
}

const CONTENT_TYPE_LABELS: Record<string, string> = {
  link: 'Link', selected_text: 'Selected text', image: 'Image',
}

// A short, readable label for a smart gesture conflict description, e.g.
// "Link →". The content type word is not localized (unlike the directional
// arrow glyphs this mirrors, which need no localization at all), since
// `findConflictingGestures` has no access to `i18n` here.
export function smartGestureKeyLabel(key: string): string {
  const [contentType, direction] = key.split(':')
  return `${CONTENT_TYPE_LABELS[contentType] ?? contentType} ` +
    `${DIRECTION_ARROWS[direction] ?? direction}`
}

export interface SmartGestureMappingActionRow {
  key: string
  direction: string
  icon: string
  actionRows: ActionRow[]
}

interface DefaultSmartGestureAction {
  command: number
  // The `IntParamInfo` value for `command`, if it takes one; stored in
  // `ActionRow.openSettingsPage` (see `hasIntParam` in
  // `gesture_action_util.ts`).
  param?: number
}

// Keep in sync with `DefaultSmartGestureMappings` in
// `smart_gesture_defaults.cc`. Right (the non-navigating copy action) always
// comes last; selected text has no 3rd destination, so its left stays
// unassigned rather than duplicating one of the above.
const DEFAULT_SMART_GESTURE_ACTIONS: Record<string, DefaultSmartGestureAction> = {
  'link:up': {
    command: GestureCommand.OPEN_LINK_IN,
    param: SmartGestureOpenInCommandParamType.NEW_BACKGROUND_TAB,
  },
  'link:down': {
    command: GestureCommand.OPEN_LINK_IN,
    param: SmartGestureOpenInCommandParamType.NEW_WINDOW,
  },
  'link:left': {
    command: GestureCommand.OPEN_LINK_IN,
    param: SmartGestureOpenInCommandParamType.NEW_FOREGROUND_TAB,
  },
  'link:right': { command: GestureCommand.COPY_LINK_ADDRESS },
  'selected_text:up': {
    command: GestureCommand.SEARCH_SELECTED_TEXT,
    param: SmartGestureSelectedTextSearchCommandParamType.BRAVE_SEARCH,
  },
  'selected_text:down': {
    command: GestureCommand.SEARCH_SELECTED_TEXT,
    param: SmartGestureSelectedTextSearchCommandParamType.ASK_BRAVE,
  },
  'selected_text:right': { command: GestureCommand.COPY_SELECTED_TEXT },
  'image:up': {
    command: GestureCommand.OPEN_IMAGE_IN,
    param: SmartGestureOpenInCommandParamType.NEW_BACKGROUND_TAB,
  },
  'image:down': {
    command: GestureCommand.OPEN_IMAGE_IN,
    param: SmartGestureOpenInCommandParamType.NEW_WINDOW,
  },
  'image:left': { command: GestureCommand.SAVE_IMAGE },
  'image:right': { command: GestureCommand.COPY_IMAGE },
}

// Builds the 4 direction rows (up/down/left/right) for one content type from
// the pref's per-key overrides, falling back to that key's own default
// action (see `DEFAULT_SMART_GESTURE_ACTIONS`), or `UNASSIGNED`, when unset.
export function buildSmartGestureMappingActionRows(
  contentType: SmartGestureContentType,
  overrides: Record<string, ActionDict[]> | undefined):
  SmartGestureMappingActionRow[] {
  return DIRECTIONS.map((direction) => {
    const key = `${contentType}:${direction}`
    const defaultAction = DEFAULT_SMART_GESTURE_ACTIONS[key]
    const actionRows = actionRowsFromDicts(
      overrides?.[key], defaultAction?.command ?? GestureCommand.UNASSIGNED)
    if (!overrides?.[key] && defaultAction?.param !== undefined) {
      actionRows[0] = { ...actionRows[0], openSettingsPage: defaultAction.param }
    }
    return { key, direction, icon: DIRECTION_ICON_NAMES[direction], actionRows }
  })
}
