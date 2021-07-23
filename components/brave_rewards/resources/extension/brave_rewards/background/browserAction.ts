/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

const getNotificationCount = (state?: RewardsExtension.State) => {
  if (!state || !state.notifications) {
    return 0
  }

  return Object.keys(state.notifications).length
}

const getColor = (tabId: number, verified: boolean, state?: RewardsExtension.State) => {
  const notificationCount = getNotificationCount(state)

  return tabId !== -1 && verified && notificationCount === 0 ? '#4C54D2' : '#FB542B'
}

const getText = (verified: boolean, tabId: number, state?: RewardsExtension.State) => {
  const notificationCount = getNotificationCount(state)

  if (notificationCount > 0) {
    return notificationCount.toString()
  }

  if (tabId !== -1 && verified) {
    return '✓️'
  }

  return ''
}

const getTabId = (tabId: number) => tabId !== -1 ? tabId : undefined

export const setBadgeText = async (state?: RewardsExtension.State, verified: boolean = false, tabId: number = -1) => {
  try {
    await chrome.browserAction.setBadgeBackgroundColor({
      color: getColor(tabId, verified, state),
      tabId: getTabId(tabId)
    })
  } catch (e) {
    console.error(e)
  }

  try {
    await chrome.browserAction.setBadgeText({
      text: getText(verified, tabId, state),
      tabId: getTabId(tabId)
    })
  } catch (e) {
    console.error(e)
  }
}
