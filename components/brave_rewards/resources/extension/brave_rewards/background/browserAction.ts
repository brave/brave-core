/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const setBadgeText = (state?: RewardsExtension.State, verified: boolean = false, tabId: number = -1) => {
  let text = ''

  if (!state) {
    return
  }

  if (state.notifications && !verified) {
    const count = Object.keys(state.notifications).length
    if (count > 0) {
      text = count.toString()
    }
  }

  let data: chrome.browserAction.BadgeTextDetails = {
    text
  }

  if (tabId !== -1) {
    data.tabId = tabId
    chrome.browserAction.setBadgeBackgroundColor({
      color: verified ? '#4C54D2' : '#FB542B',
      tabId
    })

    if (verified) {
      data.text = '✓️'
    }
  } else {
    chrome.browserAction.setBadgeBackgroundColor({
      color: '#FB542B'
    })
  }

  chrome.browserAction.setBadgeText(data)
}
