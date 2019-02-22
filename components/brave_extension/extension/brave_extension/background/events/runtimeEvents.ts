/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import windowActions from '../actions/windowActions'
import tabActions from '../actions/tabActions'
import runtimeActions from '../actions/runtimeActions'

function checkForNewWindows () {
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
}

chrome.runtime.onStartup.addListener(() => {
  runtimeActions.runtimeDidStartup()
  checkForNewWindows()
})

if (chrome.extension.inIncognitoContext) {
  // This is a work-around for a longstanding Chromium bug where onStartup
  // isn't called for incognito windows.  And since chrome.windows.onCreated
  // doesn't get called for the first window, we need to work around it here.
  // See https://github.com/brave/brave-browser/issues/1437 for more discussion.
  setTimeout(() => {
    checkForNewWindows()
  }, 1000)
}
