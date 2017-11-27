/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import windowActions from '../actions/windowActions'

chrome.windows.onFocusChanged.addListener((windowId) => {
  windowActions.windowFocusChanged(windowId)
})

chrome.windows.onCreated.addListener((windowId) => {
  windowActions.windowCreated(windowId)
})

chrome.windows.onRemoved.addListener((windowId) => {
  windowActions.windowRemoved(windowId)
})
