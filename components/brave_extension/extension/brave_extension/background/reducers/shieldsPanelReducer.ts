/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as shieldsPanelTypes from '../../constants/shieldsPanelTypes'
import * as windowTypes from '../../constants/windowTypes'
import * as tabTypes from '../../constants/tabTypes'
import * as webNavigationTypes from '../../constants/webNavigationTypes'
import {
  setAllowBraveShields,
  setAllowAds,
  setAllowTrackers,
  setAllowHTTPUpgradableResources,
  setAllowJavaScript,
  setAllowFingerprinting,
  setAllowCookies,
  toggleShieldsValue,
  requestShieldPanelData,
  setAllowScriptOriginsOnce
} from '../api/shieldsAPI'
import { setBadgeText, setIcon } from '../api/browserActionAPI'
import { reloadTab } from '../api/tabsAPI'
import * as shieldsPanelState from '../../state/shieldsPanelState'
import { State, Tab } from '../../types/state/shieldsPannelState'
import { Actions } from '../../types/actions/index'
import { getTotalResourcesBlocked } from '../../helpers/shieldsUtils'

const updateShieldsIconBadgeText = (state: State) => {
  const tabId: number = shieldsPanelState.getActiveTabId(state)
  const tab: Tab = state.tabs[tabId]
  if (tab) {
    const total = getTotalResourcesBlocked(tab)
    // do not show any badge if there are no blocked items
    setBadgeText(tabId, total > 99 ? '99+' : total > 0 ? total.toString() : '')
  }
}

const updateShieldsIconImage = (state: State) => {
  const tabId: number = shieldsPanelState.getActiveTabId(state)
  const tab: Tab = state.tabs[tabId]
  if (tab) {
    const url: string = tab.url
    const isShieldsActive: boolean = state.tabs[tabId].braveShields !== 'block'
    setIcon(url, tabId, isShieldsActive)
  }
}

const updateShieldsIcon = (state: State) => {
  updateShieldsIconBadgeText(state)
  updateShieldsIconImage(state)
}

const focusedWindowChanged = (state: State, windowId: number): State => {
  if (windowId !== -1) {
    state = shieldsPanelState.updateFocusedWindow(state, windowId)
    if (shieldsPanelState.getActiveTabId(state)) {
      requestShieldPanelData(shieldsPanelState.getActiveTabId(state))
      updateShieldsIcon(state)
    } else {
      console.warn('no tab id so cannot request shield data from window focus change!')
    }
  }
  return state
}

const updateActiveTab = (state: State, windowId: number, tabId: number): State => {
  requestShieldPanelData(tabId)
  return shieldsPanelState.updateActiveTab(state, windowId, tabId)
}

export default function shieldsPanelReducer (state: State = { tabs: {}, windows: {}, currentWindowId: -1 }, action: Actions) {
  switch (action.type) {
    case webNavigationTypes.ON_COMMITTED:
      {
        if (action.isMainFrame) {
          state = shieldsPanelState.resetBlockingStats(state, action.tabId)
          state = shieldsPanelState.resetBlockingResources(state, action.tabId)
          state = shieldsPanelState.resetNoScriptInfo(state, action.tabId, new window.URL(action.url).origin)
        }
        break
      }
    case windowTypes.WINDOW_REMOVED:
      {
        state = shieldsPanelState.removeWindowInfo(state, action.windowId)
        break
      }
    case windowTypes.WINDOW_CREATED:
      {
        if (action.window.focused || Object.keys(state.windows).length === 0) {
          state = focusedWindowChanged(state, action.window.id)
        }
        break
      }
    case windowTypes.WINDOW_FOCUS_CHANGED:
      {
        state = focusedWindowChanged(state, action.windowId)
        break
      }
    case tabTypes.ACTIVE_TAB_CHANGED:
      {
        const windowId: number = action.windowId
        const tabId: number = action.tabId
        state = updateActiveTab(state, windowId, tabId)
        updateShieldsIcon(state)
        break
      }
    case tabTypes.TAB_DATA_CHANGED:
      {
        const tab: chrome.tabs.Tab = action.tab
        if (tab.active && tab.id) {
          state = updateActiveTab(state, tab.windowId, tab.id)
          updateShieldsIcon(state)
        }
        break
      }
    case tabTypes.TAB_CREATED:
      {
        const tab: chrome.tabs.Tab = action.tab
        if (!tab) {
          break
        }

        if (tab.active && tab.id) {
          state = updateActiveTab(state, tab.windowId, tab.id)
          updateShieldsIcon(state)
        }
        break
      }
    case shieldsPanelTypes.SHIELDS_PANEL_DATA_UPDATED:
      {
        state = shieldsPanelState.updateTabShieldsData(state, action.details.id, action.details)
        updateShieldsIcon(state)
        break
      }
    case shieldsPanelTypes.SHIELDS_TOGGLED:
      {
        const tabId: number = shieldsPanelState.getActiveTabId(state)
        const tabData = shieldsPanelState.getActiveTabData(state)
        if (!tabData) {
          console.error('Active tab not found')
          break
        }
        setAllowBraveShields(tabData.origin, action.setting)
          .then(() => {
            reloadTab(tabId, true).catch(() => {
              console.error('Tab reload was not successful')
            })
            requestShieldPanelData(shieldsPanelState.getActiveTabId(state))
          })
          .catch(() => {
            console.error('Could not set shields')
          })
        state = shieldsPanelState
          .updateTabShieldsData(state, tabId, { braveShields: action.setting })
        break
      }
    case shieldsPanelTypes.HTTPS_EVERYWHERE_TOGGLED:
      {
        const tabData = shieldsPanelState.getActiveTabData(state)
        if (!tabData) {
          console.error('Active tab not found')
          break
        }

        setAllowHTTPUpgradableResources(tabData.origin, toggleShieldsValue(action.setting))
          .then(() => {
            requestShieldPanelData(shieldsPanelState.getActiveTabId(state))
            reloadTab(tabData.id, true).catch(() => {
              console.error('Tab reload was not successful')
            })
          })
          .catch(() => {
            console.error('Could not set HTTPS Everywhere setting')
          })
        break
      }
    case shieldsPanelTypes.JAVASCRIPT_TOGGLED:
      {
        const tabData = shieldsPanelState.getActiveTabData(state)
        if (!tabData) {
          console.error('Active tab not found')
          break
        }
        setAllowJavaScript(tabData.origin, toggleShieldsValue(tabData.javascript))
          .then(() => {
            requestShieldPanelData(shieldsPanelState.getActiveTabId(state))
            reloadTab(tabData.id, true).catch(() => {
              console.error('Tab reload was not successful')
            })
          })
          .catch(() => {
            console.error('Could not set JavaScript setting')
          })
        break
      }
    case shieldsPanelTypes.RESOURCE_BLOCKED:
      {
        const tabId: number = action.details.tabId
        const currentTabId: number = shieldsPanelState.getActiveTabId(state)
        state = shieldsPanelState.updateResourceBlocked(
          state, tabId, action.details.blockType, action.details.subresource)
        if (tabId === currentTabId) {
          updateShieldsIconBadgeText(state)
        }
        break
      }
    case shieldsPanelTypes.BLOCK_ADS_TRACKERS:
      {
        const tabId: number = shieldsPanelState.getActiveTabId(state)
        const tabData = shieldsPanelState.getActiveTabData(state)

        if (!tabData) {
          console.error('Active tab not found')
          break
        }

        const setting = toggleShieldsValue(action.setting)
        const p1 = setAllowAds(tabData.origin, setting)
          .catch(() => {
            console.error('Could not set ad block setting')
          })
        const p2 = setAllowTrackers(tabData.origin, setting)
          .catch(() => {
            console.error('Could not set tracking protection setting')
          })
        Promise.all([p1, p2])
          .then(() => {
            reloadTab(tabId, true).catch(() => {
              console.error('Tab reload was not successful')
            })
            requestShieldPanelData(shieldsPanelState.getActiveTabId(state))
          })
          .catch(() => {
            console.error('Could not set blockers for tracking')
          })
        break
      }
    case shieldsPanelTypes.CONTROLS_TOGGLED:
      {
        const tabId: number = shieldsPanelState.getActiveTabId(state)
        state = shieldsPanelState
          .updateTabShieldsData(state, tabId, { controlsOpen: action.setting })
        break
      }
    case shieldsPanelTypes.BLOCK_FINGERPRINTING:
      {
        const tabData = shieldsPanelState.getActiveTabData(state)
        if (!tabData) {
          console.error('Active tab not found')
          break
        }
        setAllowFingerprinting(tabData.origin, action.setting)
          .then(() => {
            requestShieldPanelData(shieldsPanelState.getActiveTabId(state))
            reloadTab(tabData.id, true).catch(() => {
              console.error('Tab reload was not successful')
            })
          })
          .catch(() => {
            console.error('Could not set fingerprinting setting')
          })
        break
      }
    case shieldsPanelTypes.BLOCK_COOKIES:
      {
        const tabData = shieldsPanelState.getActiveTabData(state)
        if (!tabData) {
          console.error('Active tab not found')
          break
        }
        setAllowCookies(tabData.origin, action.setting)
          .then(() => {
            if (action.setting === 'block') {
              chrome.cookies.getAll({ domain: tabData.origin },
                function (cookies) {
                  cookies.forEach(function (cookie) {
                    chrome.cookies.remove({ 'url': 'http://' + cookie.domain + cookie.path, 'name': cookie.name })
                    chrome.cookies.remove({ 'url': 'https://' + cookie.domain + cookie.path, 'name': cookie.name })
                  })
                }
              )
              chrome.tabs.executeScript(tabData.id, {
                code: 'try { window.sessionStorage.clear(); } catch(e) {}'})
              // clearing localStorage may fail with SecurityError if third-
              // party cookies are already blocked, but that's okay
              chrome.tabs.executeScript(tabData.id, {
                code: 'try { window.localStorage.clear(); } catch(e) {}'})
            }
            requestShieldPanelData(shieldsPanelState.getActiveTabId(state))
            reloadTab(tabData.id, true).catch(() => {
              console.error('Tab reload was not successful')
            })
          })
          .catch(() => {
            console.error('Could not set cookies setting')
          })
        break
      }
    case shieldsPanelTypes.ALLOW_SCRIPT_ORIGINS_ONCE:
      {
        const tabData = shieldsPanelState.getActiveTabData(state)
        if (!tabData) {
          console.error('Active tab not found')
          break
        }
        setAllowScriptOriginsOnce(action.origins, tabData.id)
          .then(() => {
            requestShieldPanelData(shieldsPanelState.getActiveTabId(state))
            reloadTab(tabData.id, true).catch(() => {
              console.error('Tab reload was not successful')
            })
          })
          .catch(() => {
            console.error('Could not set allow script origins once')
          })
        break
      }
    case shieldsPanelTypes.CHANGE_NO_SCRIPT_SETTINGS:
      {
        const tabId: number = shieldsPanelState.getActiveTabId(state)
        state = shieldsPanelState.changeNoScriptSettings(state, tabId, action.origin)
        break
      }
    case shieldsPanelTypes.CHANGE_ALL_NO_SCRIPT_SETTINGS:
      {
        const tabId: number = shieldsPanelState.getActiveTabId(state)
        state = shieldsPanelState.changeAllNoScriptSettings(state, tabId, action.origin, action.shouldBlock)
        break
      }
  }
  return state
}
