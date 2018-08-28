/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import windowActions from '../actions/windowActions'
import tabActions from '../actions/tabActions'
import runtimeActions from '../actions/runtimeActions'

chrome.runtime.onStartup.addListener(() => {
  runtimeActions.runtimeDidStartup()
  chrome.windows.getAllAsync({ populate: true }).then((windows: chrome.windows.Window[]) => {
    windows.forEach((win: chrome.windows.Window) => {
      windowActions.windowCreated(win)
      if (win.tabs) {
        win.tabs.forEach((tab: chrome.tabs.Tab) => {
          tabActions.tabCreated(tab)
        })
      }
    })
  })
})
