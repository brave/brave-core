/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as shieldState from '../types/state/shieldsPannelState'

export const getActiveTabId: shieldState.GetActiveTabId = (state) => state.windows[state.currentWindowId]

export const getActiveTabData: shieldState.GetActiveTabData = (state) => state.tabs[getActiveTabId(state)]

export const updateActiveTab: shieldState.UpdateActiveTab = (state, windowId, tabId) => {
  const windows: shieldState.Windows = { ...state.windows }
  // TODO not sure how this is working? we first assign windowId and then we overrider it with tabId?
  windows[windowId] = windows[windowId] // TODO @bbondy we try to assign object to number, I needed to remove || {}
  windows[windowId] = tabId
  return { ...state, windows }
}

export const removeWindowInfo: shieldState.RemoveWindowInfo = (state, windowId) => {
  const windows: shieldState.Windows = { ...state.windows }
  delete windows[windowId]
  return { ...state, windows }
}

export const updateFocusedWindow: shieldState.UpdateFocusedWindow = (state, windowId) =>
  ({ ...state, currentWindowId: windowId })

export const updateTabShieldsData: shieldState.UpdateTabShieldsData = (state, tabId, details) => {
  const tabs: shieldState.Tabs = { ...state.tabs }

  // TODO @bbondy do we need to add other default values? like httpsEverywhere, trackingProtection?
  tabs[tabId] = { ...{
    adsBlocked: 0,
    trackingProtectionBlocked: 0,
    httpsEverywhereRedirected: 0,
    javascriptBlocked: 0,
    shieldsEnabled: 'allow',
    adsTrackers: 'allow',
    controlsOpen: true
  },
    ...tabs[tabId],
    ...details
  }
  state = { ...state, tabs }
  return state
}

export const updateResourceBlocked: shieldState.UpdateResourceBlocked = (state, tabId, blockType) => {
  const tabs: shieldState.Tabs = { ...state.tabs }
  tabs[tabId] = { ...{ adsBlocked: 0, trackingProtectionBlocked: 0, httpsEverywhereRedirected: 0, javascriptBlocked: 0 }, ...tabs[tabId] }
  if (blockType === 'adBlock') {
    tabs[tabId].adsBlocked++
  } else if (blockType === 'trackingProtection') {
    tabs[tabId].trackingProtectionBlocked++
  } else if (blockType === 'httpsEverywhere') {
    tabs[tabId].httpsEverywhereRedirected++
  } else if (blockType === 'javascript') {
    tabs[tabId].javascriptBlocked++
  }
  return { ...state, tabs }
}

export const resetBlockingStats: shieldState.ResetBlockingStats = (state, tabId) => {
  const tabs: shieldState.Tabs = { ...state.tabs }
  tabs[tabId] = { ...tabs[tabId], ...{ adsBlocked: 0, trackingProtectionBlocked: 0, httpsEverywhereRedirected: 0, javascriptBlocked: 0 } }
  tabs[tabId].adsBlocked = 0
  tabs[tabId].trackingProtectionBlocked = 0
  tabs[tabId].httpsEverywhereRedirected = 0
  tabs[tabId].javascriptBlocked = 0
  return { ...state, tabs }
}
