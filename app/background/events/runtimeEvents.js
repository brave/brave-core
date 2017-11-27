/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import windowActions from '../actions/windowActions'
import tabActions from '../actions/tabActions'

chrome.runtime.onStartup.addListener((e) => {
  chrome.windows.getAllAsync({ populate: true }).then((windows) => {
    windows.forEach((win) => {
      windowActions.windowCreated(win)
      win.tabs.forEach((tab) => {
        tabActions.tabCreated(tab)
      })
    })
  })
})
