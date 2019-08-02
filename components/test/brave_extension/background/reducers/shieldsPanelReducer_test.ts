/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// Types
import * as types from '../../../../brave_extension/extension/brave_extension/constants/shieldsPanelTypes'
import * as windowTypes from '../../../../brave_extension/extension/brave_extension/constants/windowTypes'
import * as tabTypes from '../../../../brave_extension/extension/brave_extension/constants/tabTypes'
import * as webNavigationTypes from '../../../../brave_extension/extension/brave_extension/constants/webNavigationTypes'
import { State } from '../../../../brave_extension/extension/brave_extension/types/state/shieldsPannelState'
import { ShieldDetails } from '../../../../brave_extension/extension/brave_extension/types/actions/shieldsPanelActions'

// APIs
import * as shieldsAPI from '../../../../brave_extension/extension/brave_extension/background/api/shieldsAPI'
import * as tabsAPI from '../../../../brave_extension/extension/brave_extension/background/api/tabsAPI'
import * as browserActionAPI from '../../../../brave_extension/extension/brave_extension/background/api/browserActionAPI'

// Reducers
import shieldsPanelReducer from '../../../../brave_extension/extension/brave_extension/background/reducers/shieldsPanelReducer'

// State helpers
import * as shieldsPanelState from '../../../../brave_extension/extension/brave_extension/state/shieldsPanelState'
import * as noScriptState from '../../../../brave_extension/extension/brave_extension/state/noScriptState'

// Utils
import { initialState } from '../../../testData'
import * as deepFreeze from 'deep-freeze-node'
import * as actions from '../../../../brave_extension/extension/brave_extension/actions/shieldsPanelActions'

describe('braveShieldsPanelReducer', () => {
  it('should handle initial state', () => {
    expect(shieldsPanelReducer(undefined, actions.blockAdsTrackers('allow')))
      .toEqual(initialState.shieldsPanel)
  })

  describe('ON_COMMITTED', () => {
    let spy: jest.SpyInstance
    let resetNoScriptInfoSpy: jest.SpyInstance
    let resetBlockingResourcesSpy: jest.SpyInstance
    const tabId = 1
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
        windowId: windowId
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
    const state = deepFreeze({
      ...initialState.shieldsPanel,
      windows: {
        1: tabId
      },
      tabs: {
        [tabId]: { url: 'https://brave.com' }
      }
    })
    beforeEach(() => {
      updateFocusedWindowSpy = jest.spyOn(shieldsPanelState, 'updateFocusedWindow')
      requestShieldPanelDataSpy = jest.spyOn(shieldsAPI, 'requestShieldPanelData')
    })
    afterEach(() => {
      updateFocusedWindowSpy.mockRestore()
      requestShieldPanelDataSpy.mockRestore()
    })
    it('calls shieldsPanelState.updateFocusedWindow', () => {
      shieldsPanelReducer(state, { type: windowTypes.WINDOW_FOCUS_CHANGED, windowId: windowId })
      expect(updateFocusedWindowSpy).toBeCalledTimes(1)
      expect(updateFocusedWindowSpy.mock.calls[0][1]).toBe(windowId)
    })
    it('calls shieldsPanelState.requestShieldPanelDataSpy ', () => {
      shieldsPanelReducer(state, { type: windowTypes.WINDOW_FOCUS_CHANGED, windowId: windowId })
      expect(requestShieldPanelDataSpy).toBeCalledWith(tabId)
    })
  })

  describe('TAB_DATA_CHANGED', () => {
    let updateActiveTabSpy: jest.SpyInstance
    const windowId = 1
    const tabId = 2
    const state = deepFreeze({ ...initialState.shieldsPanel, windows: { 1: tabId }, tabs: {} })
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
        tab: {
          active: true,
          id: tabId,
          windowId: windowId,
          index: 1,
          pinned: false,
          highlighted: false,
          incognito: false,
          selected: false,
          discarded: false,
          autoDiscardable: false
        },
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
          active: false,
          id: tabId,
          windowId: windowId,
          index: 1,
          pinned: false,
          highlighted: false,
          incognito: false,
          selected: false,
          discarded: false,
          autoDiscardable: false
        },
        changeInfo: {}
      })
      expect(updateActiveTabSpy).not.toBeCalled()
    })
  })

  describe('TAB_CREATED', () => {
    const windowId = 1
    const tabId = 2
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
          active: true,
          id: tabId,
          windowId: windowId,
          index: 1,
          pinned: false,
          highlighted: false,
          incognito: false,
          selected: false,
          discarded: false,
          autoDiscardable: false
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
          active: false,
          id: tabId,
          windowId: windowId,
          index: 1,
          pinned: false,
          highlighted: false,
          incognito: false,
          selected: false,
          discarded: false,
          autoDiscardable: false
        }
      })
      expect(updateActiveTabSpy).not.toBeCalled()
    })
  })

  const origin = 'https://brave.com'
  const state: State = deepFreeze({
    persistentData: {},
    tabs: {
      2: {
        origin,
        hostname: 'brave.com',
        adsBlocked: 0,
        controlsOpen: true,
        braveShields: 'allow',
        trackersBlocked: 0,
        httpsRedirected: 0,
        javascriptBlocked: 0,
        fingerprintingBlocked: 0,
        id: 2,
        httpUpgradableResources: 'block',
        javascript: 'block',
        trackers: 'block',
        ads: 'block',
        fingerprinting: 'block',
        cookies: 'block',
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
  describe('SHIELDS_PANEL_DATA_UPDATED', () => {
    it('updates state detail', () => {
      const tabId = 2
      const details: ShieldDetails = {
        id: tabId,
        hostname: 'brave.com',
        origin: 'brave.com',
        ads: 'block',
        trackers: 'block',
        httpUpgradableResources: 'block',
        javascript: 'block',
        fingerprinting: 'block',
        cookies: 'block'
      }
      expect(
        shieldsPanelReducer(initialState.shieldsPanel, {
          type: types.SHIELDS_PANEL_DATA_UPDATED,
          details
        })).toEqual({
          currentWindowId: -1,
          persistentData: {
            isFirstAccess: true
          },
          tabs: {
            [tabId]: {
              adsBlocked: 0,
              trackersBlocked: 0,
              httpsRedirected: 0,
              javascriptBlocked: 0,
              fingerprintingBlocked: 0,
              hostname: 'brave.com',
              origin: 'brave.com',
              id: tabId,
              ads: 'block',
              trackers: 'block',
              httpUpgradableResources: 'block',
              javascript: 'block',
              fingerprinting: 'block',
              cookies: 'block',
              controlsOpen: true,
              braveShields: 'allow',
              noScriptInfo: {},
              adsBlockedResources: [],
              fingerprintingBlockedResources: [],
              httpsRedirectedResources: [],
              trackersBlockedResources: []
            }
          },
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
          type: types.SHIELDS_TOGGLED,
          setting: 'allow'
        })).toEqual(state)
      expect(setAllowBraveShieldsSpy).toBeCalledWith(origin, 'allow')
    })
  })

  describe('HTTPS_EVERYWHERE_TOGGLED', () => {
    let reloadTabSpy: jest.SpyInstance
    let setAllowHTTPUpgradableResourcesSpy: jest.SpyInstance
    beforeEach(() => {
      reloadTabSpy = jest.spyOn(tabsAPI, 'reloadTab')
      setAllowHTTPUpgradableResourcesSpy = jest.spyOn(shieldsAPI, 'setAllowHTTPUpgradableResources')
    })
    afterEach(() => {
      reloadTabSpy.mockRestore()
      setAllowHTTPUpgradableResourcesSpy.mockRestore()
    })
    it('should call setAllowHTTPUpgradableResources', () => {
      expect(
        shieldsPanelReducer(state, {
          type: types.HTTPS_EVERYWHERE_TOGGLED,
          setting: 'block'
        })).toEqual(state)
      expect(setAllowHTTPUpgradableResourcesSpy).toBeCalledWith(origin, 'allow')
    })
  })

  describe('JAVASCRIPT_TOGGLED', () => {
    let reloadTabSpy: jest.SpyInstance
    let setAllowJavaScriptSpy: jest.SpyInstance
    beforeEach(() => {
      reloadTabSpy = jest.spyOn(tabsAPI, 'reloadTab')
      setAllowJavaScriptSpy = jest.spyOn(shieldsAPI, 'setAllowJavaScript')
    })
    afterEach(() => {
      reloadTabSpy.mockRestore()
      setAllowJavaScriptSpy.mockRestore()
    })
    it('should call setAllowJavaScript', () => {
      expect(
        shieldsPanelReducer(state, {
          type: types.JAVASCRIPT_TOGGLED,
          setting: 'allow'
        })).toEqual(state)
      expect(setAllowJavaScriptSpy).toBeCalledWith(origin, 'allow')
    })
  })

  describe('BLOCK_FINGERPRINTING', () => {
    let reloadTabSpy: jest.SpyInstance
    let setAllowFingerprintingSpy: jest.SpyInstance
    beforeEach(() => {
      reloadTabSpy = jest.spyOn(tabsAPI, 'reloadTab')
      setAllowFingerprintingSpy = jest.spyOn(shieldsAPI, 'setAllowFingerprinting')
    })
    afterEach(() => {
      reloadTabSpy.mockRestore()
      setAllowFingerprintingSpy.mockRestore()
    })
    it('should call setAllowFingerprinting', () => {
      expect(
        shieldsPanelReducer(state, {
          type: types.BLOCK_FINGERPRINTING,
          setting: 'allow'
        })).toEqual(state)
      expect(setAllowFingerprintingSpy).toBeCalledWith(origin, 'allow')
    })
  })

  describe('BLOCK_COOKIES', () => {
    let reloadTabSpy: jest.SpyInstance
    let setAllowCookiesSpy: jest.SpyInstance
    beforeEach(() => {
      reloadTabSpy = jest.spyOn(tabsAPI, 'reloadTab')
      setAllowCookiesSpy = jest.spyOn(shieldsAPI, 'setAllowCookies')
    })
    afterEach(() => {
      reloadTabSpy.mockRestore()
      setAllowCookiesSpy.mockRestore()
    })
    it('should call setAllowCookies', () => {
      expect(
        shieldsPanelReducer(state, {
          type: types.BLOCK_COOKIES,
          setting: 'allow'
        })).toEqual(state)
      expect(setAllowCookiesSpy).toBeCalledWith(origin, 'allow')
    })
  })

  describe('RESOURCE_BLOCKED', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(browserActionAPI, 'setBadgeText')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('badge text update should include all resource types', () => {
      const stateWithBlockStats: State = {
        persistentData: {},
        tabs: {
          2: {
            origin,
            hostname: 'brave.com',
            url: 'https://brave.com',
            adsBlocked: 1,
            controlsOpen: true,
            braveShields: 'allow',
            trackersBlocked: 2,
            httpsRedirected: 3,
            javascriptBlocked: 1,
            fingerprintingBlocked: 5,
            id: 2,
            httpUpgradableResources: 'block',
            javascript: 'block',
            trackers: 'block',
            ads: 'block',
            fingerprinting: 'block',
            cookies: 'block',
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
      }
      shieldsPanelReducer(stateWithBlockStats, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'javascript',
          tabId: 2,
          subresource: 'https://a.com/index.js'
        }
      })
      expect(spy).toBeCalledTimes(1)
      expect(spy.mock.calls[0][1]).toBe('12')
    })
    it('increments for JS blocking', () => {
      let nextState = shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'javascript',
          tabId: 2,
          subresource: 'https://test.brave.com/index.js'
        }
      })

      expect(nextState).toEqual({
        currentWindowId: 1,
        persistentData: {},
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 0,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 1,
            fingerprintingBlocked: 0,
            controlsOpen: true,
            braveShields: 'allow',
            httpUpgradableResources: 'block',
            id: 2,
            javascript: 'block',
            trackers: 'block',
            ads: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            noScriptInfo: {
              'https://test.brave.com/index.js': { actuallyBlocked: true, willBlock: true, userInteracted: false }
            },
            trackersBlockedResources: [],
            adsBlockedResources: [],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: []
          }
        },
        windows: {
          1: 2
        }
      })
    })

    it('increments JS blocking consecutively', () => {
      let nextState = shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'javascript',
          tabId: 2,
          subresource: 'https://a.com/index.js'
        }
      })
      expect(nextState).toEqual({
        currentWindowId: 1,
        persistentData: {},
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 0,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 1,
            fingerprintingBlocked: 0,
            controlsOpen: true,
            braveShields: 'allow',
            httpUpgradableResources: 'block',
            id: 2,
            javascript: 'block',
            trackers: 'block',
            ads: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            noScriptInfo: {
              'https://a.com/index.js': { actuallyBlocked: true, willBlock: true, userInteracted: false }
            },
            trackersBlockedResources: [],
            adsBlockedResources: [],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: []
          }
        },
        windows: {
          1: 2
        }
      })

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'javascript',
          tabId: 2,
          subresource: 'https://b.com/index.js'
        }
      })
      expect(nextState).toEqual({
        currentWindowId: 1,
        persistentData: {},
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 0,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 2,
            fingerprintingBlocked: 0,
            controlsOpen: true,
            braveShields: 'allow',
            httpUpgradableResources: 'block',
            id: 2,
            javascript: 'block',
            trackers: 'block',
            ads: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            noScriptInfo: {
              'https://a.com/index.js': { actuallyBlocked: true, willBlock: true, userInteracted: false },
              'https://b.com/index.js': { actuallyBlocked: true, willBlock: true, userInteracted: false }
            },
            trackersBlockedResources: [],
            adsBlockedResources: [],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: []
          }
        },
        windows: {
          1: 2
        }
      })

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'javascript',
          tabId: 2,
          subresource: 'https://a.com/index.js'
        }
      })
      expect(nextState).toEqual({
        currentWindowId: 1,
        persistentData: {},
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 0,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 2,
            fingerprintingBlocked: 0,
            controlsOpen: true,
            braveShields: 'allow',
            httpUpgradableResources: 'block',
            id: 2,
            javascript: 'block',
            trackers: 'block',
            ads: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            noScriptInfo: {
              'https://a.com/index.js': { actuallyBlocked: true, willBlock: true, userInteracted: false },
              'https://b.com/index.js': { actuallyBlocked: true, willBlock: true, userInteracted: false }
            },
            trackersBlockedResources: [],
            adsBlockedResources: [],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: []
          }
        },
        windows: {
          1: 2
        }
      })
    })

    it('increments for fingerprinting blocked', () => {
      let nextState = shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'fingerprinting',
          tabId: 2,
          subresource: 'https://test.brave.com'
        }
      })
      expect(nextState).toEqual({
        currentWindowId: 1,
        persistentData: {},
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 0,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 1,
            controlsOpen: true,
            braveShields: 'allow',
            httpUpgradableResources: 'block',
            id: 2,
            javascript: 'block',
            trackers: 'block',
            ads: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            noScriptInfo: {},
            trackersBlockedResources: [],
            adsBlockedResources: [],
            fingerprintingBlockedResources: [ 'https://test.brave.com' ],
            httpsRedirectedResources: []
          }
        },
        windows: {
          1: 2
        }
      })
    })

    it('increases same count consecutively', () => {
      let nextState = shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'ads',
          tabId: 2,
          subresource: 'https://test.brave.com'
        }
      })
      expect(nextState).toEqual({
        currentWindowId: 1,
        persistentData: {},
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 1,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0,
            controlsOpen: true,
            braveShields: 'allow',
            httpUpgradableResources: 'block',
            id: 2,
            javascript: 'block',
            trackers: 'block',
            ads: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            noScriptInfo: {},
            trackersBlockedResources: [],
            adsBlockedResources: [ 'https://test.brave.com' ],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: []
          }
        },
        windows: {
          1: 2
        }
      })

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'ads',
          tabId: 2,
          subresource: 'https://test2.brave.com'
        }
      })
      expect(nextState).toEqual({
        currentWindowId: 1,
        persistentData: {},
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 2,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0,
            controlsOpen: true,
            braveShields: 'allow',
            httpUpgradableResources: 'block',
            id: 2,
            javascript: 'block',
            trackers: 'block',
            ads: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            noScriptInfo: {},
            trackersBlockedResources: [],
            adsBlockedResources: [
              'https://test.brave.com',
              'https://test2.brave.com'
            ],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: []
          }
        },
        windows: {
          1: 2
        }
      })
    })
    it('increases same count consecutively without duplicates', () => {
      const tabId = 2
      let nextState = shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'ads',
          tabId: tabId,
          subresource: 'https://test.brave.com'
        }
      })
      expect(nextState.tabs[tabId].adsBlockedResources).toEqual(
        [ 'https://test.brave.com' ]
      )

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'ads',
          tabId: tabId,
          subresource: 'https://test2.brave.com'
        }
      })
      expect(nextState.tabs[tabId].adsBlockedResources).toEqual(
        [
          'https://test.brave.com',
          'https://test2.brave.com'
        ]
      )

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'ads',
          tabId: tabId,
          subresource: 'https://test2.brave.com'
        }
      })
      expect(nextState.tabs[tabId].adsBlockedResources).toEqual(
        [
          'https://test.brave.com',
          'https://test2.brave.com'
        ]
      )
    })

    it('increases different tab counts separately', () => {
      let nextState = deepFreeze(shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'ads',
          tabId: 2,
          subresource: 'https://test.brave.com'
        }
      }))
      expect(nextState).toEqual({
        currentWindowId: 1,
        persistentData: {},
        tabs: {
          2: {
            adsBlocked: 1,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0,
            origin: 'https://brave.com',
            hostname: 'brave.com',
            controlsOpen: true,
            braveShields: 'allow',
            httpUpgradableResources: 'block',
            id: 2,
            javascript: 'block',
            trackers: 'block',
            ads: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            noScriptInfo: {},
            trackersBlockedResources: [],
            adsBlockedResources: [
              'https://test.brave.com'
            ],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: []
          }
        },
        windows: {
          1: 2
        }
      })

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'ads',
          tabId: 3,
          subresource: 'https://test.brave.com'
        }
      })

      expect(nextState).toEqual({
        currentWindowId: 1,
        persistentData: {},
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 1,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0,
            controlsOpen: true,
            braveShields: 'allow',
            httpUpgradableResources: 'block',
            id: 2,
            javascript: 'block',
            trackers: 'block',
            ads: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            noScriptInfo: {},
            trackersBlockedResources: [],
            adsBlockedResources: [ 'https://test.brave.com' ],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: []
          },
          3: {
            adsBlocked: 1,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0,
            noScriptInfo: {},
            trackersBlockedResources: [],
            adsBlockedResources: [ 'https://test.brave.com' ],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: []
          }
        },
        windows: {
          1: 2
        }
      })
    })
    it('increases different resource types separately', () => {
      let nextState = deepFreeze(shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'ads',
          tabId: 2,
          subresource: 'https://test.brave.com'
        }
      }))
      expect(nextState).toEqual({
        currentWindowId: 1,
        persistentData: {},
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 1,
            trackersBlocked: 0,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0,
            controlsOpen: true,
            braveShields: 'allow',
            httpUpgradableResources: 'block',
            id: 2,
            javascript: 'block',
            trackers: 'block',
            ads: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            noScriptInfo: {},
            adsBlockedResources: [ 'https://test.brave.com' ],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: [],
            trackersBlockedResources: []
          }
        },
        windows: {
          1: 2
        }
      })

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'trackers',
          tabId: 2,
          subresource: 'https://test.brave.com'
        }
      })

      expect(nextState).toEqual({
        currentWindowId: 1,
        persistentData: {},
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 1,
            trackersBlocked: 1,
            httpsRedirected: 0,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0,
            controlsOpen: true,
            braveShields: 'allow',
            httpUpgradableResources: 'block',
            id: 2,
            javascript: 'block',
            trackers: 'block',
            ads: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            noScriptInfo: {},
            trackersBlockedResources: [ 'https://test.brave.com' ],
            adsBlockedResources: [ 'https://test.brave.com' ],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: []
          }
        },
        windows: {
          1: 2
        }
      })

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'httpUpgradableResources',
          tabId: 2,
          subresource: 'https://test.brave.com'
        }
      })
      expect(nextState).toEqual({
        currentWindowId: 1,
        persistentData: {},
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 1,
            trackersBlocked: 1,
            httpsRedirected: 1,
            javascriptBlocked: 0,
            fingerprintingBlocked: 0,
            controlsOpen: true,
            braveShields: 'allow',
            httpUpgradableResources: 'block',
            id: 2,
            javascript: 'block',
            trackers: 'block',
            ads: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            noScriptInfo: {},
            trackersBlockedResources: [ 'https://test.brave.com' ],
            adsBlockedResources: [ 'https://test.brave.com' ],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: [ 'https://test.brave.com' ]
          }
        },
        windows: {
          1: 2
        }
      })
      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'javascript',
          tabId: 2,
          subresource: 'https://test.brave.com/index.js'
        }
      })
      expect(nextState).toEqual({
        currentWindowId: 1,
        persistentData: {},
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 1,
            trackersBlocked: 1,
            httpsRedirected: 1,
            javascriptBlocked: 1,
            fingerprintingBlocked: 0,
            controlsOpen: true,
            braveShields: 'allow',
            httpUpgradableResources: 'block',
            id: 2,
            javascript: 'block',
            trackers: 'block',
            ads: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            noScriptInfo: {
              'https://test.brave.com/index.js': { actuallyBlocked: true, willBlock: true, userInteracted: false }
            },
            trackersBlockedResources: [ 'https://test.brave.com' ],
            adsBlockedResources: [ 'https://test.brave.com' ],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: [ 'https://test.brave.com' ]
          }
        },
        windows: {
          1: 2
        }
      })
      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'fingerprinting',
          tabId: 2,
          subresource: 'https://test.brave.com'
        }
      })
      expect(nextState).toEqual({
        currentWindowId: 1,
        persistentData: {},
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 1,
            trackersBlocked: 1,
            httpsRedirected: 1,
            javascriptBlocked: 1,
            fingerprintingBlocked: 1,
            controlsOpen: true,
            braveShields: 'allow',
            httpUpgradableResources: 'block',
            id: 2,
            javascript: 'block',
            trackers: 'block',
            ads: 'block',
            fingerprinting: 'block',
            cookies: 'block',
            noScriptInfo: {
              'https://test.brave.com/index.js': { actuallyBlocked: true, willBlock: true, userInteracted: false }
            },
            trackersBlockedResources: [ 'https://test.brave.com' ],
            adsBlockedResources: [ 'https://test.brave.com' ],
            fingerprintingBlockedResources: [ 'https://test.brave.com' ],
            httpsRedirectedResources: [ 'https://test.brave.com' ]
          }
        },
        windows: {
          1: 2
        }
      })
    })
  })

  describe('BLOCK_ADS_TRACKERS', () => {
    let reloadTabSpy: jest.SpyInstance
    let setAllowAdsSpy: jest.SpyInstance
    let setAllowTrackersSpy: jest.SpyInstance
    beforeEach(() => {
      reloadTabSpy = jest.spyOn(tabsAPI, 'reloadTab')
      setAllowAdsSpy = jest.spyOn(shieldsAPI, 'setAllowAds')
      setAllowTrackersSpy = jest.spyOn(shieldsAPI, 'setAllowTrackers')
    })
    afterEach(() => {
      reloadTabSpy.mockRestore()
      setAllowAdsSpy.mockRestore()
      setAllowTrackersSpy.mockRestore()
    })
    it('should call setAllowAds and setAllowTrackers', () => {
      expect(
        shieldsPanelReducer(state, {
          type: types.BLOCK_ADS_TRACKERS,
          setting: 'allow'
        })).toEqual(state)
      expect(setAllowAdsSpy).toBeCalledWith(origin, 'block')
      expect(setAllowTrackersSpy).toBeCalledWith(origin, 'block')
    })
  })

  describe('ALLOW_SCRIPT_ORIGINS_ONCE', () => {
    let reloadTabSpy: jest.SpyInstance
    let setAllowScriptOriginsOnceSpy: jest.SpyInstance
    beforeEach(() => {
      reloadTabSpy = jest.spyOn(tabsAPI, 'reloadTab')
      setAllowScriptOriginsOnceSpy = jest.spyOn(shieldsAPI, 'setAllowScriptOriginsOnce')
    })
    afterEach(() => {
      reloadTabSpy.mockRestore()
      setAllowScriptOriginsOnceSpy.mockRestore()
    })
    it('should call setAllowScriptOriginsOnce', () => {
      const tabId = 2
      expect(
        shieldsPanelReducer(state, {
          type: types.ALLOW_SCRIPT_ORIGINS_ONCE
        })).toEqual(state)
      expect(setAllowScriptOriginsOnceSpy).toBeCalledWith([], tabId)
    })
  })

  describe('SET_ADVANCED_VIEW_FIRST_ACCESS', () => {
    let updatePersistentDataSpy: jest.SpyInstance
    beforeEach(() => {
      updatePersistentDataSpy = jest.spyOn(shieldsPanelState, 'updatePersistentData')
    })
    afterEach(() => {
      updatePersistentDataSpy.mockRestore()
    })
    it('should call updatePersistentData', () => {
      const isFirstAccess: boolean = false
      const persistentData = { isFirstAccess }
      expect(
        shieldsPanelReducer(state, {
          type: types.SET_ADVANCED_VIEW_FIRST_ACCESS
        })).toEqual({ ...state, persistentData })
      expect(updatePersistentDataSpy).toBeCalledWith(state, persistentData)
    })
  })
})
