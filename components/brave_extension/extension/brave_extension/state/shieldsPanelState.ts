/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as shieldState from '../types/state/shieldsPannelState'
import { unique } from '../helpers/arrayUtils'
import { getTotalResourcesBlocked } from '../helpers/shieldsUtils'
import { setBadgeText, setIcon } from '../background/api/browserActionAPI'
import { requestShieldPanelData } from '../background/api/shieldsAPI'

export const getActiveTabId: shieldState.GetActiveTabId = (state) => state.windows[state.currentWindowId]

export const getActiveTabData: shieldState.GetActiveTabData = (state) => state.tabs[getActiveTabId(state)]

export const isShieldsActive = (state: shieldState.State, tabId: number): boolean => {
  if (!state.tabs[tabId]) {
    return false
  }
  return state.tabs[tabId].braveShields !== 'block'
}

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
    javascriptBlockedResources: [],
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
      javascriptBlockedResources: [],
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
    const origin = new window.URL(subresource).origin + '/'
    tabs[tabId].noScriptInfo = { ...tabs[tabId].noScriptInfo }
    tabs[tabId].noScriptInfo[origin] = { ...{ actuallyBlocked: true, willBlock: true } }
    tabs[tabId].javascriptBlockedResources = unique([ ...tabs[tabId].javascriptBlockedResources, subresource ])
    tabs[tabId].javascriptBlocked = tabs[tabId].javascriptBlockedResources.length
  } else if (blockType === 'fingerprinting') {
    tabs[tabId].fingerprintingBlockedResources = unique([ ...tabs[tabId].fingerprintingBlockedResources, subresource ])
    tabs[tabId].fingerprintingBlocked = tabs[tabId].fingerprintingBlockedResources.length
  }

  return { ...state, tabs }
}

export const changeNoScriptSettings: shieldState.ChangeNoScriptSettings = (state, tabId, origin) => {
  const tabs: shieldState.Tabs = { ...state.tabs }
  tabs[tabId] = { ...{ adsBlocked: 0, trackersBlocked: 0, httpsRedirected: 0, javascriptBlocked: 0, fingerprintingBlocked: 0, noScriptInfo: {} }, ...tabs[tabId] }
  tabs[tabId].noScriptInfo[origin].willBlock = !tabs[tabId].noScriptInfo[origin].willBlock
  return { ...state, tabs }
}

export const resetNoScriptInfo: shieldState.ResetNoScriptInfo = (state, tabId, newOrigin) => {
  const tabs: shieldState.Tabs = { ...state.tabs }
  if (newOrigin !== tabs[tabId].origin) { // navigate away
    tabs[tabId].noScriptInfo = {}
  }
  Object.keys(tabs[tabId].noScriptInfo).map(key => {
    tabs[tabId].noScriptInfo[key].actuallyBlocked = false
    // only keep entries which users want to allow
    if (tabs[tabId].noScriptInfo[key].willBlock) {
      delete tabs[tabId].noScriptInfo[key]
    }
  })
  return { ...state, tabs }
}

export const changeAllNoScriptSettings: shieldState.ChangeAllNoScriptSettings = (state, tabId, shouldBlock) => {
  const tabs: shieldState.Tabs = { ...state.tabs }
  Object.keys(tabs[tabId].noScriptInfo).map(key => {
    tabs[tabId].noScriptInfo[key].willBlock = shouldBlock
  })
  return { ...state, tabs }
}

export const resetBlockingStats: shieldState.ResetBlockingStats = (state, tabId) => {
  const tabs: shieldState.Tabs = { ...state.tabs }
  tabs[tabId] = { ...tabs[tabId], ...{ adsBlocked: 0, trackersBlocked: 0, httpsRedirected: 0, javascriptBlocked: 0, fingerprintingBlocked: 0 } }
  return { ...state, tabs }
}

export const resetBlockingResources: shieldState.ResetBlockingResources = (state, tabId) => {
  const tabs: shieldState.Tabs = { ...state.tabs }
  tabs[tabId] = { ...tabs[tabId], ...{ adsBlockedResources: [], trackersBlockedResources: [], httpsRedirectedResources: [], javascriptBlockedResources: [], fingerprintingBlockedResources: [] } }
  return { ...state, tabs }
}

export const updateShieldsIconBadgeText: shieldState.UpdateShieldsIconBadgeText = (state) => {
  const tabId: number = getActiveTabId(state)
  const tab: shieldState.Tab = state.tabs[tabId]
  if (tab) {
    const total = getTotalResourcesBlocked(tab)
    // do not show any badge if there are no blocked items
    setBadgeText(tabId, total > 99 ? '99+' : total > 0 ? total.toString() : '')
  }
}

export const updateShieldsIconImage: shieldState.UpdateShieldsIconImage = (state) => {
  const tabId: number = getActiveTabId(state)
  const tab: shieldState.Tab = state.tabs[tabId]
  if (tab) {
    const url: string = tab.url
    const isShieldsActive: boolean = state.tabs[tabId].braveShields !== 'block'
    setIcon(url, tabId, isShieldsActive)
  }
}

export const updateShieldsIcon: shieldState.UpdateShieldsIcon = (state) => {
  updateShieldsIconBadgeText(state)
  updateShieldsIconImage(state)
}

export const focusedWindowChanged: shieldState.FocusedWindowChanged = (state, windowId) => {
  if (windowId !== -1) {
    state = updateFocusedWindow(state, windowId)
    if (getActiveTabId(state)) {
      requestShieldPanelData(getActiveTabId(state))
      updateShieldsIcon(state)
    } else {
      console.warn('no tab id so cannot request shield data from window focus change!')
    }
  }
  return state
}

export const requestDataAndUpdateActiveTab: shieldState.RequestDataAndUpdateActiveTab = (state, windowId, tabId) => {
  requestShieldPanelData(tabId)
  return updateActiveTab(state, windowId, tabId)
}
