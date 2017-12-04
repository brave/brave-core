/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

export const getActiveTabId = (state) => state.windows[state.currentWindowId]

export const getActiveTabData = (state) => state.tabs[getActiveTabId(state)]

export const updateActiveTab = (state, windowId, tabId) => {
  const windows = {...state.windows}
  windows[windowId] = windows[windowId] || {}
  windows[windowId] = tabId
  return {...state, windows}
}

export const removeWindowInfo = (state, windowId) => {
  const windows = {...state.windows}
  delete windows[windowId]
  return {...state, windows}
}

export const updateFocusedWindow = (state, windowId) =>
  ({...state, currentWindowId: windowId})

export const updateTabShieldsData = (state, tabId, details) => {
  const tabs = {...state.tabs}
  tabs[tabId] = {...{
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
  state = {...state, tabs}
  return state
}

export const updateResourceBlocked = (state, tabId, blockType) => {
  const tabs = {...state.tabs}
  tabs[tabId] = {...{ adsBlocked: 0, trackingProtectionBlocked: 0, httpsEverywhereRedirected: 0, javascriptBlocked: 0 }, ...tabs[tabId]}
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

export const resetBlockingStats = (state, tabId) => {
  const tabs = {...state.tabs}
  tabs[tabId] = {...tabs[tabId], ...{ adsBlocked: 0, trackingProtectionBlocked: 0, httpsEverywhereRedirected: 0, javascriptBlocked: 0 }}
  tabs[tabId].adsBlocked = 0
  tabs[tabId].trackingProtectionBlocked = 0
  tabs[tabId].httpsEverywhereRedirected = 0
  tabs[tabId].javascriptBlocked = 0
  return { ...state, tabs }
}
