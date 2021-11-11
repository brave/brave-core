/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../constants/windowTypes'

interface WindowFocusChangedReturn {
  type: types.WINDOW_FOCUS_CHANGED
  windowId: number
}

export type WindowFocusChanged = (windowId: number) => WindowFocusChangedReturn

interface WindowCreatedReturn {
  type: types.WINDOW_CREATED
  window: chrome.windows.Window
}

export type WindowCreated = (window: chrome.windows.Window) => WindowCreatedReturn

interface WindowRemovedReturn {
  type: types.WINDOW_REMOVED
  windowId: number
}

export type WindowRemoved = (windowId: number) => WindowRemovedReturn

export type windowActions =
  WindowFocusChangedReturn |
  WindowCreatedReturn |
  WindowRemovedReturn
