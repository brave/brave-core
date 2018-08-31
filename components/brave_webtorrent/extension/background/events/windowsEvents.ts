/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import windowActions from '../actions/windowActions'

chrome.windows.onFocusChanged.addListener((windowId: number) => {
  windowActions.windowFocusChanged(windowId)
})

chrome.windows.onCreated.addListener((window: chrome.windows.Window) => {
  windowActions.windowCreated(window)
})

chrome.windows.onRemoved.addListener((windowId: number) => {
  windowActions.windowRemoved(windowId)
})
