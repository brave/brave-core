/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/windowTypes'

// TODO @bbondy what else do we have inhere?
export interface Window {
  id: number
}

export interface WindowFocusChanged {
  (windowId: number): {
    type: types.WINDOW_FOCUS_CHANGED,
    windowId: number
  }
}

export interface WindowCreated {
  (window: Window): {
    type: types.WINDOW_CREATED,
    window: Window
  }
}

export interface WindowRemoved {
  (windowId: number): {
    type: types.WINDOW_REMOVED,
    windowId: number
  }
}
