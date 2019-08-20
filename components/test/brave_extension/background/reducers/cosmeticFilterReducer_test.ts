/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

// Types
import * as shieldPanelTypes from '../../../../brave_extension/extension/brave_extension/constants/shieldsPanelTypes'
import * as cosmeticFilterTypes from '../../../../brave_extension/extension/brave_extension/constants/cosmeticFilterTypes'
import * as windowTypes from '../../../../brave_extension/extension/brave_extension/constants/windowTypes'
import * as tabTypes from '../../../../brave_extension/extension/brave_extension/constants/tabTypes'
import * as webNavigationTypes from '../../../../brave_extension/extension/brave_extension/constants/webNavigationTypes'
import { State } from '../../../../brave_extension/extension/brave_extension/types/state/shieldsPannelState'
import { ShieldDetails } from '../../../../brave_extension/extension/brave_extension/types/actions/shieldsPanelActions'

// APIs
import * as shieldsAPI from '../../../../brave_extension/extension/brave_extension/background/api/shieldsAPI'
import * as cosmeticFilterAPI from '../../../../brave_extension/extension/brave_extension/background/api/cosmeticFilterAPI'
import * as tabsAPI from '../../../../brave_extension/extension/brave_extension/background/api/tabsAPI'

// Reducers
import shieldsPanelReducer from '../../../../brave_extension/extension/brave_extension/background/reducers/shieldsPanelReducer'
import cosmeticFilterReducer from '../../../../brave_extension/extension/brave_extension/background/reducers/cosmeticFilterReducer'

// State helpers
import * as shieldsPanelState from '../../../../brave_extension/extension/brave_extension/state/shieldsPanelState'
import * as noScriptState from '../../../../brave_extension/extension/brave_extension/state/noScriptState'

// Helpers
import { initialState } from '../../../testData'
import * as deepFreeze from 'deep-freeze-node'
import * as actions from '../../../../brave_extension/extension/brave_extension/actions/shieldsPanelActions'

const origin = 'https://brave.com'
const windowId = 1
const tabId = 2

const details: ShieldDetails = {
  id: tabId,
  origin,
  hostname: 'brave.com',
  httpUpgradableResources: 'block',
  javascript: 'block',
  trackers: 'block',
  ads: 'block',
  fingerprinting: 'block',
  cookies: 'block'
}

const tab: chrome.tabs.Tab = {
  active: true,
  id: tabId,
  windowId,
  index: 1,
  pinned: false,
  highlighted: false,
  incognito: false,
  selected: false,
  discarded: false,
  autoDiscardable: false
}

const state: State = deepFreeze({
  persistentData: {
    isFirstAccess: true
  },
  tabs: {
    2: {
      ...details,
      adsBlocked: 0,
      controlsOpen: true,
      braveShields: 'allow',
      trackersBlocked: 0,
      httpsRedirected: 0,
      javascriptBlocked: 0,
      fingerprintingBlocked: 0,
      noScriptInfo: {},
      adsBlockedResources: [],
      fingerprintingBlockedResources: [],
      httpsRedirectedResources: [],
      trackersBlockedResources: []
    }
  },
  windows: {
    1: 2
  },
  currentWindowId: 1
})

describe('cosmeticFilterReducer', () => {
  it('should handle initial state', () => {
    // avoid printing error logs to the test console.
    // this is expected since state is undefined but we want to avoid polluting the test logs
    console.error = () => ''
    expect(shieldsPanelReducer(undefined, actions.allowScriptOriginsOnce()))
      .toEqual(initialState.cosmeticFilter)
  })
  describe('ON_COMMITTED', () => {
    const tabId = 1
    let spy: jest.SpyInstance
    let resetNoScriptInfoSpy: jest.SpyInstance
    let resetBlockingResourcesSpy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(shieldsPanelState, 'resetBlockingStats')
      resetNoScriptInfoSpy = jest.spyOn(noScriptState, 'resetNoScriptInfo')
      resetBlockingResourcesSpy = jest.spyOn(shieldsPanelState, 'resetBlockingResources')
    })
    afterEach(() => {
      spy.mockRestore()
      resetNoScriptInfoSpy.mockRestore()
      resetBlockingResourcesSpy.mockRestore()
    })
    it('calls resetBlockingStats when isMainFrame is true', () => {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_COMMITTED,
        tabId: tabId,
        url: 'https://www.brave.com',
        isMainFrame: true
      })
      expect(spy).toBeCalledTimes(1)
      expect(spy.mock.calls[0][1]).toBe(tabId)
    })
    it('does not call resetBlockingStats when isMainFrame is false', () => {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_COMMITTED,
        tabId: tabId,
        url: 'https://www.brave.com',
        isMainFrame: false
      })
      expect(spy).not.toBeCalled()
    })
    it('calls resetNoScriptInfo when isMainFrame is true', () => {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_COMMITTED,
        tabId: tabId,
        url: 'https://www.brave.com',
        isMainFrame: true
      })
      expect(resetNoScriptInfoSpy).toBeCalledTimes(1)
      expect(resetNoScriptInfoSpy.mock.calls[0][1]).toBe(tabId)
      expect(resetNoScriptInfoSpy.mock.calls[0][2]).toBe('https://www.brave.com')
    })
    it('does not call resetNoScriptInfo when isMainFrame is false', () => {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_COMMITTED,
        tabId: tabId,
        url: 'https://www.brave.com',
        isMainFrame: false
      })
      expect(resetNoScriptInfoSpy).not.toBeCalled()
    })
    it('calls resetBlockingResources when isMainFrame is true', () => {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_COMMITTED,
        tabId: tabId,
        url: 'https://www.brave.com',
        isMainFrame: true
      })
      expect(spy).toBeCalledTimes(1)
      expect(spy.mock.calls[0][1]).toBe(tabId)
    })
    it('does not call resetBlockingResources when isMainFrame is false', () => {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_COMMITTED,
        tabId: tabId,
        url: 'https://www.brave.com',
        isMainFrame: false
      })
      expect(spy).not.toBeCalled()
    })
  })
  describe('WINDOW_REMOVED', () => {
    const windowId = 1
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(shieldsPanelState, 'removeWindowInfo')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls shieldsPanelState.removeWindowInfo', () => {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: windowTypes.WINDOW_REMOVED,
        windowId
      })
      expect(spy).toBeCalledTimes(1)
      expect(spy.mock.calls[0][1]).toBe(windowId)
    })
  })
  describe('WINDOW_FOCUS_CHANGED', () => {
    const windowId = 1
    const tabId = 2
    let updateFocusedWindowSpy: jest.SpyInstance
    let requestShieldPanelDataSpy: jest.SpyInstance
    beforeEach(() => {
      updateFocusedWindowSpy = jest.spyOn(shieldsPanelState, 'updateFocusedWindow')
      requestShieldPanelDataSpy = jest.spyOn(shieldsAPI, 'requestShieldPanelData')
      const state = deepFreeze({
        ...initialState.shieldsPanel,
        windows: {
          1: tabId
        },
        tabs: {
          [tabId]: { url: 'https://brave.com' }
        }
      })
      shieldsPanelReducer(state, {
        type: windowTypes.WINDOW_FOCUS_CHANGED,
        windowId: windowId
      })
    })
    afterEach(() => {
      updateFocusedWindowSpy.mockRestore()
      requestShieldPanelDataSpy.mockRestore()
    })
    it('calls shieldsPanelState.updateFocusedWindow', () => {
      expect(updateFocusedWindowSpy).toBeCalledTimes(1)
      expect(updateFocusedWindowSpy.mock.calls[0][1]).toBe(windowId)
    })
    it('calls shieldsPanelState.requestShieldPanelDataSpy ', () => {
      expect(requestShieldPanelDataSpy).toBeCalledWith(tabId)
    })
  })
  describe('TAB_DATA_CHANGED', () => {
    const windowId = 1
    const tabId = 2
    const state = deepFreeze({ ...initialState.shieldsPanel, windows: { 1: tabId }, tabs: {} })
    let updateActiveTabSpy: jest.SpyInstance
    beforeEach(() => {
      updateActiveTabSpy = jest.spyOn(shieldsPanelState, 'updateActiveTab')
    })
    afterEach(() => {
      updateActiveTabSpy.mockRestore()
    })
    it('calls shieldsPanelState.updateActiveTab when the tab is active', () => {
      shieldsPanelReducer(state, {
        type: tabTypes.TAB_DATA_CHANGED,
        tabId: tabId,
        tab,
        changeInfo: {}
      })
      expect(updateActiveTabSpy).toBeCalledTimes(1)
      expect(updateActiveTabSpy.mock.calls[0][1]).toBe(windowId)
      expect(updateActiveTabSpy.mock.calls[0][2]).toBe(tabId)
    })
    it('does not call shieldsPanelState.updateActiveTab when the tab is not active', () => {
      shieldsPanelReducer(state, {
        type: tabTypes.TAB_DATA_CHANGED,
        tabId: tabId,
        tab: {
          ...tab,
          active: false
        },
        changeInfo: {}
      })
      expect(updateActiveTabSpy).not.toBeCalled()
    })
  })
  describe('TAB_CREATED', () => {
    const state = {
      ...initialState.shieldsPanel,
      windows: {
        1: tabId
      },
      tabs: {}
    }
    let updateActiveTabSpy: jest.SpyInstance
    beforeEach(() => {
      updateActiveTabSpy = jest.spyOn(shieldsPanelState, 'updateActiveTab')
    })
    afterEach(() => {
      updateActiveTabSpy.mockRestore()
    })
    it('calls shieldsPanelState.updateActiveTab when the tab is active', () => {
      shieldsPanelReducer(state, {
        type: tabTypes.TAB_CREATED,
        tab: {
          ...tab,
          active: true
        }
      })
      expect(updateActiveTabSpy).toBeCalledTimes(1)
      expect(updateActiveTabSpy.mock.calls[0][1]).toBe(windowId)
      expect(updateActiveTabSpy.mock.calls[0][2]).toBe(tabId)
    })
    it('does not call shieldsPanelState.updateActiveTab when the tab is not active', () => {
      shieldsPanelReducer(state, {
        type: tabTypes.TAB_CREATED,
        tab: {
          ...tab,
          active: false
        }
      })
      expect(updateActiveTabSpy).not.toBeCalled()
    })
  })
  describe('SHIELDS_PANEL_DATA_UPDATED', () => {
    it('updates state detail', () => {
      expect(
          shieldsPanelReducer(initialState.shieldsPanel, {
            type: shieldPanelTypes.SHIELDS_PANEL_DATA_UPDATED,
            details
          })).toEqual({
            ...state,
            currentWindowId: -1,
            windows: {}
          })
    })
  })
  describe('SHIELDS_TOGGLED', () => {
    let reloadTabSpy: jest.SpyInstance
    let setAllowBraveShieldsSpy: jest.SpyInstance
    beforeEach(() => {
      reloadTabSpy = jest.spyOn(tabsAPI, 'reloadTab')
      setAllowBraveShieldsSpy = jest.spyOn(shieldsAPI, 'setAllowBraveShields')
    })
    afterEach(() => {
      reloadTabSpy.mockRestore()
      setAllowBraveShieldsSpy.mockRestore()
    })
    it('should call setAllowBraveShields', () => {
      expect(
        shieldsPanelReducer(state, {
          type: shieldPanelTypes.SHIELDS_TOGGLED,
          setting: 'allow'
        })).toEqual(state)
      expect(setAllowBraveShieldsSpy).toBeCalledWith(origin, 'allow')
    })
  })
  describe('SITE_COSMETIC_FILTER_REMOVED', () => {
    let removeSiteFilterSpy: jest.SpyInstance
    beforeEach(() => {
      removeSiteFilterSpy = jest.spyOn(cosmeticFilterAPI, 'removeSiteFilter')
    })
    afterEach(() => {
      removeSiteFilterSpy.mockRestore()
    })
    it('should call removeSiteFilter', () => {
      expect(
        cosmeticFilterReducer(state, {
          type: cosmeticFilterTypes.SITE_COSMETIC_FILTER_REMOVED,
          origin
        })).toEqual(state)
    })
  })
  describe('ALL_COSMETIC_FILTERS_REMOVED', () => {
    let removeSiteFilterSpy: jest.SpyInstance
    beforeEach(() => {
      removeSiteFilterSpy = jest.spyOn(cosmeticFilterAPI, 'removeAllFilters')
    })
    afterEach(() => {
      removeSiteFilterSpy.mockRestore()
    })
    it('should call removeAllFilters', () => {
      expect(
        cosmeticFilterReducer(state, {
          type: cosmeticFilterTypes.ALL_COSMETIC_FILTERS_REMOVED
        })).toEqual(state)
    })
  })
  describe('SITE_COSMETIC_FILTER_ADDED', () => {
    let removeSiteFilterSpy: jest.SpyInstance
    beforeEach(() => {
      removeSiteFilterSpy = jest.spyOn(cosmeticFilterAPI, 'addSiteCosmeticFilter')
    })
    afterEach(() => {
      removeSiteFilterSpy.mockRestore()
    })
    it('should call addSiteCosmeticFilter', () => {
      const cssfilter = '#test'
      expect(
        cosmeticFilterReducer(state, {
          type: cosmeticFilterTypes.SITE_COSMETIC_FILTER_ADDED,
          origin,
          cssfilter
        })).toEqual(state)
    })
  })
})
