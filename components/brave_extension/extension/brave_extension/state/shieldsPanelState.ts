/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Types
import * as shieldState from '../types/state/shieldsPannelState'

// Helpers
import { unique } from '../helpers/arrayUtils'
import { filterNoScriptInfoByWillBlockState } from '../helpers/noScriptUtils'

export const getActiveTabId: shieldState.GetActiveTabId = (state) => state.windows[state.currentWindowId]

export const getActiveTabData: shieldState.GetActiveTabData = (state) => state.tabs[getActiveTabId(state)]

export const updateActiveTab: shieldState.UpdateActiveTab = (state, windowId, tabId) => {
  let windows: shieldState.Windows = { ...state.windows } || {}
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

  tabs[tabId] = { ...{
    adsBlocked: 0,
    trackersBlocked: 0,
    httpsRedirected: 0,
    javascriptBlocked: 0,
    fingerprintingBlocked: 0,
    braveShields: 'allow',
    ads: 'allow',
    trackers: 'allow',
    httpUpgradableResources: 'allow',
    javascript: 'allow',
    fingerprinting: 'allow',
    controlsOpen: true,
    noScriptInfo: {},
    adsBlockedResources: [],
    trackersBlockedResources: [],
    httpsRedirectedResources: [],
    fingerprintingBlockedResources: []
  },
    ...tabs[tabId],
    ...details
  }
  state = { ...state, tabs }
  return state
}

export const updateResourceBlocked: shieldState.UpdateResourceBlocked = (state, tabId, blockType, subresource) => {
  const tabs: shieldState.Tabs = { ...state.tabs }
  tabs[tabId] = {
    ...{
      adsBlocked: 0,
      trackersBlocked: 0,
      httpsRedirected: 0,
      javascriptBlocked: 0,
      fingerprintingBlocked: 0,
      noScriptInfo: {},
      adsBlockedResources: [],
      trackersBlockedResources: [],
      httpsRedirectedResources: [],
      fingerprintingBlockedResources: []
    },
    ...tabs[tabId]
  }

  if (blockType === 'ads') {
    tabs[tabId].adsBlockedResources = unique([ ...tabs[tabId].adsBlockedResources, subresource ])
    tabs[tabId].adsBlocked = tabs[tabId].adsBlockedResources.length
  } else if (blockType === 'trackers') {
    tabs[tabId].trackersBlockedResources = unique([ ...tabs[tabId].trackersBlockedResources, subresource ])
    tabs[tabId].trackersBlocked = tabs[tabId].trackersBlockedResources.length
  } else if (blockType === 'httpUpgradableResources') {
    tabs[tabId].httpsRedirectedResources = unique([ ...tabs[tabId].httpsRedirectedResources, subresource ])
    tabs[tabId].httpsRedirected = tabs[tabId].httpsRedirectedResources.length
  } else if (blockType === 'javascript') {
    tabs[tabId].noScriptInfo = { ...tabs[tabId].noScriptInfo }
    tabs[tabId].noScriptInfo[subresource] = { ...{ actuallyBlocked: true, willBlock: true, userInteracted: false } }
    tabs[tabId].javascriptBlocked = filterNoScriptInfoByWillBlockState(Object.entries(tabs[tabId].noScriptInfo), true).length
  } else if (blockType === 'fingerprinting') {
    tabs[tabId].fingerprintingBlockedResources = unique([ ...tabs[tabId].fingerprintingBlockedResources, subresource ])
    tabs[tabId].fingerprintingBlocked = tabs[tabId].fingerprintingBlockedResources.length
  }

  return { ...state, tabs }
}

export const resetBlockingStats: shieldState.ResetBlockingStats = (state, tabId) => {
  const tabs: shieldState.Tabs = { ...state.tabs }
  tabs[tabId] = { ...tabs[tabId], ...{ adsBlocked: 0, trackersBlocked: 0, httpsRedirected: 0, javascriptBlocked: 0, fingerprintingBlocked: 0 } }
  return { ...state, tabs }
}

export const resetBlockingResources: shieldState.ResetBlockingResources = (state, tabId) => {
  const tabs: shieldState.Tabs = { ...state.tabs }
  tabs[tabId] = { ...tabs[tabId], ...{ adsBlockedResources: [], trackersBlockedResources: [], httpsRedirectedResources: [], fingerprintingBlockedResources: [] } }
  return { ...state, tabs }
}
