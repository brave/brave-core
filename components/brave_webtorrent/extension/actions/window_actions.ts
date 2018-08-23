/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/window_types'

export const windowCreated = (window: chrome.windows.Window) => action(types.WINDOW_CREATED, {
  window
})

export const windowRemoved = (windowId: number) => action(types.WINDOW_REMOVED, {
  windowId
})

export const windowFocusChanged = (windowId: number) => action(types.WINDOW_FOCUS_CHANGED, {
  windowId
})
