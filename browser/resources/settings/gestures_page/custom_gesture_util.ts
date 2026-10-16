/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import {GestureCommand} from './gesture_command.js'
import {svgPathData, fitToViewBox} from './custom_gesture_svg_path.js'
import type {CustomGestureActionRow, CustomGestureEntry} from './gesture_rows.js'
import {actionRowsFromDicts} from './gesture_rows.js'

export const GESTURES_CUSTOM_GESTURES_PREF = 'brave.gestures.custom_gestures'

const CUSTOM_GESTURE_PREVIEW_SIZE = 32
const CUSTOM_GESTURE_PREVIEW_PADDING = 4

// Identifies the drawn-custom-gesture draft as a "row" where one is
// otherwise expected to be a persisted object.
export const DRAWN_CUSTOM_GESTURE_ROW = Symbol('drawnCustomGesture')

// Builds every saved custom gesture's row, including its preview path, from
// the pref's stored entries.
export function buildCustomGestureActionRows(
  customGestures: CustomGestureEntry[] | undefined): CustomGestureActionRow[] {
  return (customGestures ?? []).map((customGesture) => ({
    id: customGesture.id,
    previewSvgPathData: customGesture.points.length >= 2
      ? svgPathData(
        fitToViewBox(customGesture.points, CUSTOM_GESTURE_PREVIEW_SIZE,
          CUSTOM_GESTURE_PREVIEW_PADDING))
      : '',
    actionRows: actionRowsFromDicts(
      customGesture.actions, GestureCommand.UNASSIGNED),
  }))
}
