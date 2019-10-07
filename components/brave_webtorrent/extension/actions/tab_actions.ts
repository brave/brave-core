/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { action } from 'typesafe-actions'

// Constants
import { types } from '../constants/tab_types'

export const tabUpdated = (tabId: number,
    changeInfo: chrome.webNavigation.WebNavigationFramedCallbackDetails) =>
  action(types.TAB_UPDATED, {
    tabId, changeInfo
  })

export const tabRemoved = (tabId: number) => action(types.TAB_REMOVED, {
  tabId
})

export const tabRetrieved = (tab: chrome.tabs.Tab) => action(types.TAB_RETRIEVED, {
  tab
})
