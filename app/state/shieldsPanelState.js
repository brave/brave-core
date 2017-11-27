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
  tabs[tabId] = {...{ adsBlocked: 0, trackingProtectionBlocked: 0 }, ...tabs[tabId], ...details}
  state = {...state, tabs}
  return state
}

export const updateResourceBlocked = (state, tabId, blockType) => {
  const tabs = {...state.tabs}
  tabs[tabId] = {...{ adsBlocked: 0, trackingProtectionBlocked: 0 }, ...tabs[tabId]}
  if (blockType === 'adBlock') {
    tabs[tabId].adsBlocked++
  } else if (blockType === 'trackingProtection') {
    tabs[tabId].trackingProtectionBlocked++
  }
  return { ...state, tabs }
}
