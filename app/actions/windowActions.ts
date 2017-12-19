/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/windowTypes'
import * as actions from '../types/actions/windowActions'

export const windowFocusChanged: actions.WindowFocusChanged = (windowId) => {
  return { type: types.WINDOW_FOCUS_CHANGED, windowId }
}

export const windowCreated: actions.WindowCreated = (window) => {
  return { type: types.WINDOW_CREATED, window }
}

export const windowRemoved: actions.WindowRemoved = (windowId) => {
  return { type: types.WINDOW_REMOVED, windowId }
}
