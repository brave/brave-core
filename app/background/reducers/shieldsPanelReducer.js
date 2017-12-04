/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as shieldsPanelTypes from '../../constants/shieldsPanelTypes'
import * as windowTypes from '../../constants/windowTypes'
import * as tabTypes from '../../constants/tabTypes'
import * as webNavigationTypes from '../../constants/webNavigationTypes'
import {
  setAllowAdBlock,
  setAllowTrackingProtection,
  setAllowHTTPSEverywhere,
  setAllowJavaScript,
  toggleShieldsValue,
  requestShieldPanelData
} from '../api/shieldsAPI'
import {setBadgeText} from '../api/badgeAPI'
import {reloadTab} from '../api/tabsAPI'
import * as shieldsPanelState from '../../state/shieldsPanelState'

const updateBadgeText = (state) => {
  const tabId = shieldsPanelState.getActiveTabId(state)
  if (state.tabs[tabId]) {
    setBadgeText(state.tabs[tabId].adsBlocked + state.tabs[tabId].trackingProtectionBlocked)
  }
}

const focusedWindowChanged = (state, windowId) => {
  if (windowId !== -1) {
    state = shieldsPanelState.updateFocusedWindow(state, windowId)
    if (shieldsPanelState.getActiveTabId(state)) {
      requestShieldPanelData(shieldsPanelState.getActiveTabId(state))
      updateBadgeText(state)
    } else {
      console.warn('no tab id so cannot request shield data from window focus change!')
    }
  }
  return state
}

const updateActiveTab = (state, windowId, tabId) => {
  requestShieldPanelData(tabId)
  return shieldsPanelState.updateActiveTab(state, windowId, tabId)
}

export default function shieldsPanelReducer (state = {tabs: {}, windows: {}}, action) {
  switch (action.type) {
    case webNavigationTypes.ON_BEFORE_NAVIGATION:
      if (action.isMainFrame) {
        state = shieldsPanelState.resetBlockingStats(state, action.tabId)
      }
      break
    case windowTypes.WINDOW_REMOVED:
      state = shieldsPanelState.removeWindowInfo(state, action.windowId)
      break
    case windowTypes.WINDOW_CREATED:
      if (action.window.focused || state.windows.length === 0) {
        state = focusedWindowChanged(state, action.window.id)
      }
      break
    case windowTypes.WINDOW_FOCUS_CHANGED:
      state = focusedWindowChanged(state, action.windowId)
      break
    case tabTypes.ACTIVE_TAB_CHANGED: {
      const windowId = action.windowId
      const tabId = action.tabId
      state = updateActiveTab(state, windowId, tabId)
      updateBadgeText(state)
      break
    }
    case tabTypes.TAB_DATA_CHANGED:
      const tab = action.tab
      if (tab.active) {
        state = updateActiveTab(state, tab.windowId, tab.id)
        updateBadgeText(state)
      }
      break
    case tabTypes.TAB_CREATED: {
      const tab = action.tab
      if (tab.active) {
        state = updateActiveTab(state, tab.windowId, tab.id)
        updateBadgeText(state)
      }
      break
    }
    case shieldsPanelTypes.SHIELDS_PANEL_DATA_UPDATED:
      state = shieldsPanelState.updateTabShieldsData(state, action.details.id, action.details)
      break
    case shieldsPanelTypes.SHIELDS_TOGGLED: {
      const tabId = shieldsPanelState.getActiveTabId(state)
      const tabData = shieldsPanelState.getActiveTabData(state)
      const p1 = setAllowAdBlock(tabData.origin, action.setting)
        .catch(() => {
          console.error('Could not set ad block setting')
        })
      const p2 = setAllowTrackingProtection(tabData.origin, action.setting)
        .catch(() => {
          console.error('Could not set tracking protection setting')
        })
      const p3 = setAllowHTTPSEverywhere(tabData.origin, action.setting)
        .catch(() => {
          console.error('Could not set HTTPS Everywhere setting')
        })
      const p4 = setAllowJavaScript(tabData.origin, action.setting)
        .catch(() => {
          console.error('Could not set JavaScript setting')
        })
      Promise.all([p1, p2, p3, p4]).then(() => {
        reloadTab(tabId, true)
        requestShieldPanelData(shieldsPanelState.getActiveTabId(state))
      })
      state = shieldsPanelState
        .updateTabShieldsData(state, tabId, { shieldsEnabled: action.setting })
      break
    }
    case shieldsPanelTypes.AD_BLOCK_TOGGLED: {
      const tabData = shieldsPanelState.getActiveTabData(state)
      setAllowAdBlock(tabData.origin, toggleShieldsValue(tabData.adBlock))
        .then(() => {
          requestShieldPanelData(shieldsPanelState.getActiveTabId(state))
          reloadTab(tabData.id, true)
        })
        .catch(() => {
          console.error('Could not set ad block setting')
        })
      break
    }
    case shieldsPanelTypes.HTTPS_EVERYWHERE_TOGGLED: {
      const tabData = shieldsPanelState.getActiveTabData(state)
      setAllowHTTPSEverywhere(tabData.origin, toggleShieldsValue(tabData.httpsEverywhere))
        .then(() => {
          requestShieldPanelData(shieldsPanelState.getActiveTabId(state))
          reloadTab(tabData.id, true)
        })
        .catch(() => {
          console.error('Could not set HTTPS Everywhere setting')
        })
      break
    }
    case shieldsPanelTypes.JAVASCRIPT_TOGGLED: {
      const tabData = shieldsPanelState.getActiveTabData(state)
      setAllowJavaScript(tabData.origin, toggleShieldsValue(tabData.javascript))
        .then(() => {
          requestShieldPanelData(shieldsPanelState.getActiveTabId(state))
          reloadTab(tabData.id, true)
        })
        .catch(() => {
          console.error('Could not set JavaScript setting')
        })
      break
    }
    case shieldsPanelTypes.TRACKING_PROTECTION_TOGGLED:
      const tabData = shieldsPanelState.getActiveTabData(state)
      setAllowTrackingProtection(tabData.origin, toggleShieldsValue(tabData.trackingProtection))
        .then(() => {
          requestShieldPanelData(shieldsPanelState.getActiveTabId(state))
          reloadTab(tabData.id, true)
        })
        .catch(() => {
          console.error('Could not set tracking protection setting')
        })
      break
    case shieldsPanelTypes.RESOURCE_BLOCKED: {
      const tabId = action.details.tabId
      const currentTabId = shieldsPanelState.getActiveTabId(state)
      state = shieldsPanelState.updateResourceBlocked(state, tabId, action.details.blockType)
      if (tabId === currentTabId) {
        updateBadgeText(state)
      }
      break
    }
    case shieldsPanelTypes.BLOCK_ADS_TRACKERS: {
      const tabId = shieldsPanelState.getActiveTabId(state)
      const tabData = shieldsPanelState.getActiveTabData(state)
      const p1 = setAllowAdBlock(tabData.origin, action.setting)
        .catch(() => {
          console.error('Could not set ad block setting')
        })
      const p2 = setAllowTrackingProtection(tabData.origin, action.setting)
        .catch(() => {
          console.error('Could not set tracking protection setting')
        })
      Promise.all([p1, p2]).then(() => {
        reloadTab(tabId, true)
        requestShieldPanelData(shieldsPanelState.getActiveTabId(state))
      })
      state = shieldsPanelState
        .updateTabShieldsData(state, tabId, { adsTrackers: action.setting })
      break
    }
    case shieldsPanelTypes.CONTROLS_TOGGLED: {
      const tabId = shieldsPanelState.getActiveTabId(state)
      state = shieldsPanelState
        .updateTabShieldsData(state, tabId, { controlsOpen: action.setting })
      break
    }
  }
  return state
}
