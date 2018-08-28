/* global describe, it, before, after, afterEach */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as sinon from 'sinon'
import * as assert from 'assert'
import * as types from '../../../../app/constants/shieldsPanelTypes'
import * as windowTypes from '../../../../app/constants/windowTypes'
import * as tabTypes from '../../../../app/constants/tabTypes'
import * as webNavigationTypes from '../../../../app/constants/webNavigationTypes'
import shieldsPanelReducer from '../../../../app/background/reducers/shieldsPanelReducer'
import * as shieldsAPI from '../../../../app/background/api/shieldsAPI'
import * as tabsAPI from '../../../../app/background/api/tabsAPI'
import * as browserActionAPI from '../../../../app/background/api/browserActionAPI'
import * as shieldsPanelState from '../../../../app/state/shieldsPanelState'
import { initialState } from '../../../testData'
import * as deepFreeze from 'deep-freeze-node'
import { ShieldDetails } from '../../../../app/types/actions/shieldsPanelActions'
import * as actions from '../../../../app/actions/shieldsPanelActions'
import { State } from '../../../../app/types/state/shieldsPannelState'

describe('braveShieldsPanelReducer', () => {
  it('should handle initial state', () => {
    assert.deepEqual(
      shieldsPanelReducer(undefined, actions.blockAdsTrackers('allow'))
    , initialState.shieldsPanel)
  })

  describe('ON_BEFORE_NAVIGATION', function () {
    before(function () {
      this.spy = sinon.spy(shieldsPanelState, 'resetBlockingStats')
      this.resetNoScriptInfoSpy = sinon.spy(shieldsPanelState, 'resetNoScriptInfo')
      this.resetBlockingResourcesSpy = sinon.spy(shieldsPanelState, 'resetBlockingResources')
      this.tabId = 1
    })
    after(function () {
      this.spy.restore()
      this.resetNoScriptInfoSpy.restore()
      this.resetBlockingResourcesSpy.restore()
    })
    afterEach(function () {
      this.spy.reset()
      this.resetNoScriptInfoSpy.reset()
    })
    it('calls resetBlockingStats when isMainFrame is true', function () {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_BEFORE_NAVIGATION,
        tabId: this.tabId,
        url: 'https://www.brave.com',
        isMainFrame: true
      })
      assert.equal(this.spy.calledOnce, true)
      assert.equal(this.spy.getCall(0).args[1], this.tabId)
    })
    it('does not call resetBlockingStats when isMainFrame is false', function () {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_BEFORE_NAVIGATION,
        tabId: this.tabId,
        url: 'https://www.brave.com',
        isMainFrame: false
      })
      assert.equal(this.spy.notCalled, true)
    })
    it('calls resetNoScriptInfo when isMainFrame is true', function () {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_BEFORE_NAVIGATION,
        tabId: this.tabId,
        url: 'https://www.brave.com',
        isMainFrame: true
      })
      assert.equal(this.resetNoScriptInfoSpy.calledOnce, true)
      assert.equal(this.resetNoScriptInfoSpy.getCall(0).args[1], this.tabId)
      assert.equal(this.resetNoScriptInfoSpy.getCall(0).args[2], 'https://www.brave.com')
    })
    it('does not call resetNoScriptInfo when isMainFrame is false', function () {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_BEFORE_NAVIGATION,
        tabId: this.tabId,
        url: 'https://www.brave.com',
        isMainFrame: false
      })
      assert.equal(this.resetNoScriptInfoSpy.notCalled, true)
    })
    it('calls resetBlockingResources when isMainFrame is true', function () {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_BEFORE_NAVIGATION,
        tabId: this.tabId,
        url: 'https://www.brave.com',
        isMainFrame: true
      })
      assert.equal(this.spy.calledOnce, true)
      assert.equal(this.spy.getCall(0).args[1], this.tabId)
    })
    it('does not call resetBlockingResources when isMainFrame is false', function () {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_BEFORE_NAVIGATION,
        tabId: this.tabId,
        url: 'https://www.brave.com',
        isMainFrame: false
      })
      assert.equal(this.spy.notCalled, true)
    })
  })

  describe('WINDOW_REMOVED', function () {
    before(function () {
      this.spy = sinon.spy(shieldsPanelState, 'removeWindowInfo')
      this.windowId = 1
    })
    after(function () {
      this.spy.restore()
    })
    it('calls shieldsPanelState.removeWindowInfo', function () {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: windowTypes.WINDOW_REMOVED,
        windowId: this.windowId
      })
      assert.equal(this.spy.calledOnce, true)
      assert.equal(this.spy.getCall(0).args[1], this.windowId)
    })
  })

  describe('WINDOW_FOCUS_CHANGED', function () {
    before(function () {
      this.updateFocusedWindowSpy = sinon.spy(shieldsPanelState, 'updateFocusedWindow')
      this.requestShieldPanelDataSpy = sinon.spy(shieldsAPI, 'requestShieldPanelData')
      this.windowId = 1
      this.tabId = 2
      const state = deepFreeze({
        ...initialState.shieldsPanel,
        windows: {
          1: this.tabId
        },
        tabs: {
          [this.tabId]: { url: 'https://brave.com' }
        }
      })
      shieldsPanelReducer(state, {
        type: windowTypes.WINDOW_FOCUS_CHANGED,
        windowId: this.windowId
      })
    })
    after(function () {
      this.updateFocusedWindowSpy.restore()
      this.requestShieldPanelDataSpy.restore()
    })
    it('calls shieldsPanelState.updateFocusedWindow', function () {
      assert.equal(this.updateFocusedWindowSpy.calledOnce, true)
      assert.equal(this.updateFocusedWindowSpy.getCall(0).args[1], this.windowId)
    })
    it('calls shieldsPanelState.requestShieldPanelDataSpy ', function () {
      assert.equal(this.requestShieldPanelDataSpy.withArgs(this.tabId).calledOnce, true)
    })
  })

  describe('TAB_DATA_CHANGED', function () {
    before(function () {
      this.updateActiveTabSpy = sinon.spy(shieldsPanelState, 'updateActiveTab')
      this.windowId = 1
      this.tabId = 2
      this.state = deepFreeze({ ...initialState.shieldsPanel, windows: { 1: this.tabId }, tabs: {} })
    })
    after(function () {
      this.updateActiveTabSpy.restore()
    })
    afterEach(function () {
      this.updateActiveTabSpy.reset()
    })
    it('calls shieldsPanelState.updateActiveTab when the tab is active', function () {
      shieldsPanelReducer(this.state, {
        type: tabTypes.TAB_DATA_CHANGED,
        tabId: this.tabId,
        tab: {
          active: true,
          id: this.tabId,
          windowId: this.windowId,
          index: 1,
          pinned: false,
          highlighted: false,
          incognito: false,
          selected: false
        },
        changeInfo: {}
      })
      assert.equal(this.updateActiveTabSpy.calledOnce, true)
      assert.equal(this.updateActiveTabSpy.getCall(0).args[1], this.windowId)
      assert.equal(this.updateActiveTabSpy.getCall(0).args[2], this.tabId)
    })
    it('does not call shieldsPanelState.updateActiveTab when the tab is not active', function () {
      shieldsPanelReducer(this.state, {
        type: tabTypes.TAB_DATA_CHANGED,
        tabId: this.tabId,
        tab: {
          active: false,
          id: this.tabId,
          windowId: this.windowId,
          index: 1,
          pinned: false,
          highlighted: false,
          incognito: false,
          selected: false
        },
        changeInfo: {}
      })
      assert.equal(this.updateActiveTabSpy.notCalled, true)
    })
  })

  describe('TAB_CREATED', function () {
    before(function () {
      this.updateActiveTabSpy = sinon.spy(shieldsPanelState, 'updateActiveTab')
      this.windowId = 1
      this.tabId = 2
      this.state = {
        ...initialState.shieldsPanel,
        windows: {
          1: this.tabId
        },
        tabs: {}
      }
    })
    after(function () {
      this.updateActiveTabSpy.restore()
    })
    afterEach(function () {
      this.updateActiveTabSpy.reset()
    })
    it('calls shieldsPanelState.updateActiveTab when the tab is active', function () {
      shieldsPanelReducer(this.state, {
        type: tabTypes.TAB_CREATED,
        tab: {
          active: true,
          id: this.tabId,
          windowId: this.windowId,
          index: 1,
          pinned: false,
          highlighted: false,
          incognito: false,
          selected: false
        }
      })
      assert.equal(this.updateActiveTabSpy.calledOnce, true)
      assert.equal(this.updateActiveTabSpy.getCall(0).args[1], this.windowId)
      assert.equal(this.updateActiveTabSpy.getCall(0).args[2], this.tabId)
    })
    it('does not call shieldsPanelState.updateActiveTab when the tab is not active', function () {
      shieldsPanelReducer(this.state, {
        type: tabTypes.TAB_CREATED,
        tab: {
          active: false,
          id: this.tabId,
          windowId: this.windowId,
          index: 1,
          pinned: false,
          highlighted: false,
          incognito: false,
          selected: false
        }
      })
      assert.equal(this.updateActiveTabSpy.notCalled, true)
    })
  })

  const origin = 'https://brave.com'
  const state: State = deepFreeze({
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
        javascriptBlockedResources: [],
        trackersBlockedResources: []
      }
    },
    windows: {
      1: 2
    },
    currentWindowId: 1
  })
  describe('SHIELDS_PANEL_DATA_UPDATED', function () {
    it('updates state detail', function () {
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
      assert.deepEqual(
        shieldsPanelReducer(initialState.shieldsPanel, {
          type: types.SHIELDS_PANEL_DATA_UPDATED,
          details
        }), {
          currentWindowId: -1,
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
              javascriptBlockedResources: [],
              trackersBlockedResources: []
            }
          },
          windows: {}
        })
    })
  })

  describe('SHIELDS_TOGGLED', function () {
    before(function () {
      this.reloadTabSpy = sinon.spy(tabsAPI, 'reloadTab')
      this.setAllowBraveShieldsSpy = sinon.spy(shieldsAPI, 'setAllowBraveShields')
    })
    after(function () {
      this.reloadTabSpy.restore()
      this.setAllowBraveShieldsSpy.restore()
    })
    it('should call setAllowBraveShields', function () {
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.SHIELDS_TOGGLED,
          setting: 'allow'
        }), state)
      assert.equal(this.setAllowBraveShieldsSpy.withArgs(origin, 'allow').calledOnce, true)
    })
  })

  describe('HTTPS_EVERYWHERE_TOGGLED', function () {
    before(function () {
      this.reloadTabSpy = sinon.spy(tabsAPI, 'reloadTab')
      this.setAllowHTTPUpgradableResourcesSpy = sinon.spy(shieldsAPI, 'setAllowHTTPUpgradableResources')
    })
    after(function () {
      this.reloadTabSpy.restore()
      this.setAllowHTTPUpgradableResourcesSpy.restore()
    })
    it('should call setAllowHTTPUpgradableResources', function () {
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.HTTPS_EVERYWHERE_TOGGLED
        }), state)
      assert.equal(this.setAllowHTTPUpgradableResourcesSpy.withArgs(origin, 'allow').calledOnce, true)
    })
  })

  describe('JAVASCRIPT_TOGGLED', function () {
    before(function () {
      this.reloadTabSpy = sinon.spy(tabsAPI, 'reloadTab')
      this.setAllowJavaScriptSpy = sinon.spy(shieldsAPI, 'setAllowJavaScript')
    })
    after(function () {
      this.reloadTabSpy.restore()
      this.setAllowJavaScriptSpy.restore()
    })
    it('should call setAllowJavaScript', function () {
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.JAVASCRIPT_TOGGLED
        }), state)
      assert.equal(this.setAllowJavaScriptSpy.withArgs(origin, 'allow').calledOnce, true)
    })
  })

  describe('BLOCK_FINGERPRINTING', function () {
    before(function () {
      this.reloadTabSpy = sinon.spy(tabsAPI, 'reloadTab')
      this.setAllowFingerprintingSpy = sinon.spy(shieldsAPI, 'setAllowFingerprinting')
    })
    after(function () {
      this.reloadTabSpy.restore()
      this.setAllowFingerprintingSpy.restore()
    })
    it('should call setAllowFingerprinting', function () {
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.BLOCK_FINGERPRINTING,
          setting: 'allow'
        }), state)
      assert.equal(this.setAllowFingerprintingSpy.withArgs(origin, 'allow').calledOnce, true)
    })
  })

  describe('BLOCK_COOKIES', function () {
    before(function () {
      this.reloadTabSpy = sinon.spy(tabsAPI, 'reloadTab')
      this.setAllowCookiesSpy = sinon.spy(shieldsAPI, 'setAllowCookies')
    })
    after(function () {
      this.reloadTabSpy.restore()
      this.setAllowCookiesSpy.restore()
    })
    it('should call setAllowCookies', function () {
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.BLOCK_COOKIES,
          setting: 'allow'
        }), state)
      assert.equal(this.setAllowCookiesSpy.withArgs(origin, 'allow').calledOnce, true)
    })
  })

  describe('RESOURCE_BLOCKED', function () {
    before(function () {
      this.spy = sinon.spy(browserActionAPI, 'setBadgeText')
    })
    after(function () {
      this.spy.restore()
    })
    it('badge text update should include all resource types', function () {
      const stateWithBlockStats: State = {
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
            javascriptBlockedResources: [],
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
      assert.equal(this.spy.calledOnce, true)
      assert.equal(this.spy.getCall(0).args[1], '12')
    })
    it('increments for JS blocking', function () {
      let nextState = shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'javascript',
          tabId: 2,
          subresource: 'https://test.brave.com/index.js'
        }
      })

      assert.deepEqual(nextState, {
        currentWindowId: 1,
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
              'https://test.brave.com/': { actuallyBlocked: true, willBlock: true }
            },
            trackersBlockedResources: [],
            adsBlockedResources: [],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [ 'https://test.brave.com/index.js' ]
          }
        },
        windows: {
          1: 2
        }
      })
    })

    it('increments JS blocking consecutively', function () {
      let nextState = shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'javascript',
          tabId: 2,
          subresource: 'https://a.com/index.js'
        }
      })
      assert.deepEqual(nextState, {
        currentWindowId: 1,
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
              'https://a.com/': { actuallyBlocked: true, willBlock: true }
            },
            trackersBlockedResources: [],
            adsBlockedResources: [],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [ 'https://a.com/index.js' ]
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
      assert.deepEqual(nextState, {
        currentWindowId: 1,
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
              'https://a.com/': { actuallyBlocked: true, willBlock: true },
              'https://b.com/': { actuallyBlocked: true, willBlock: true }
            },
            trackersBlockedResources: [],
            adsBlockedResources: [],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [
              'https://a.com/index.js',
              'https://b.com/index.js'
            ]
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
      assert.deepEqual(nextState, {
        currentWindowId: 1,
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
              'https://a.com/': { actuallyBlocked: true, willBlock: true },
              'https://b.com/': { actuallyBlocked: true, willBlock: true }
            },
            trackersBlockedResources: [],
            adsBlockedResources: [],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [
              'https://a.com/index.js',
              'https://b.com/index.js'
            ]
          }
        },
        windows: {
          1: 2
        }
      })
    })

    it('increments JS blocking consecutively without duplicates', function () {
      this.tabId = 2
      let nextState = shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'javascript',
          tabId: this.tabId,
          subresource: 'https://a.com/index.js'
        }
      })
      assert.deepEqual(
        nextState.tabs[this.tabId].javascriptBlockedResources,
        [ 'https://a.com/index.js' ]
      )

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'javascript',
          tabId: this.tabId,
          subresource: 'https://b.com/index.js'
        }
      })
      assert.deepEqual(
        nextState.tabs[this.tabId].javascriptBlockedResources,
        [
          'https://a.com/index.js',
          'https://b.com/index.js'
        ]
      )

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'javascript',
          tabId: this.tabId,
          subresource: 'https://b.com/index.js'
        }
      })
      assert.deepEqual(
        nextState.tabs[this.tabId].javascriptBlockedResources,
        [
          'https://a.com/index.js',
          'https://b.com/index.js'
        ]
      )
    })

    it('increments for fingerprinting blocked', function () {
      let nextState = shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'fingerprinting',
          tabId: 2,
          subresource: 'https://test.brave.com'
        }
      })
      assert.deepEqual(nextState, {
        currentWindowId: 1,
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
            httpsRedirectedResources: [],
            javascriptBlockedResources: []
          }
        },
        windows: {
          1: 2
        }
      })
    })

    it('increases same count consecutively', function () {
      let nextState = shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'ads',
          tabId: 2,
          subresource: 'https://test.brave.com'
        }
      })
      assert.deepEqual(nextState, {
        currentWindowId: 1,
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
            httpsRedirectedResources: [],
            javascriptBlockedResources: []
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
      assert.deepEqual(nextState, {
        currentWindowId: 1,
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
            httpsRedirectedResources: [],
            javascriptBlockedResources: []
          }
        },
        windows: {
          1: 2
        }
      })
    })
    it('increases same count consecutively without duplicates', function () {
      this.tabId = 2
      let nextState = shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'ads',
          tabId: this.tabId,
          subresource: 'https://test.brave.com'
        }
      })
      assert.deepEqual(
        nextState.tabs[this.tabId].adsBlockedResources,
        [ 'https://test.brave.com' ]
      )

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'ads',
          tabId: this.tabId,
          subresource: 'https://test2.brave.com'
        }
      })
      assert.deepEqual(
        nextState.tabs[this.tabId].adsBlockedResources,
        [
          'https://test.brave.com',
          'https://test2.brave.com'
        ]
      )

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'ads',
          tabId: this.tabId,
          subresource: 'https://test2.brave.com'
        }
      })
      assert.deepEqual(
        nextState.tabs[this.tabId].adsBlockedResources,
        [
          'https://test.brave.com',
          'https://test2.brave.com'
        ]
      )
    })

    it('increases different tab counts separately', function () {
      let nextState = deepFreeze(shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'ads',
          tabId: 2,
          subresource: 'https://test.brave.com'
        }
      }))
      assert.deepEqual(nextState, {
        currentWindowId: 1,
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
            httpsRedirectedResources: [],
            javascriptBlockedResources: []
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

      assert.deepEqual(nextState, {
        currentWindowId: 1,
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
            httpsRedirectedResources: [],
            javascriptBlockedResources: []
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
            httpsRedirectedResources: [],
            javascriptBlockedResources: []
          }
        },
        windows: {
          1: 2
        }
      })
    })
    it('increases different resource types separately', function () {
      let nextState = deepFreeze(shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'ads',
          tabId: 2,
          subresource: 'https://test.brave.com'
        }
      }))
      assert.deepEqual(nextState, {
        currentWindowId: 1,
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
            javascriptBlockedResources: [],
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

      assert.deepEqual(nextState, {
        currentWindowId: 1,
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
            httpsRedirectedResources: [],
            javascriptBlockedResources: []
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
      assert.deepEqual(nextState, {
        currentWindowId: 1,
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
            httpsRedirectedResources: [ 'https://test.brave.com' ],
            javascriptBlockedResources: []
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
      assert.deepEqual(nextState, {
        currentWindowId: 1,
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
              'https://test.brave.com/': { actuallyBlocked: true, willBlock: true }
            },
            trackersBlockedResources: [ 'https://test.brave.com' ],
            adsBlockedResources: [ 'https://test.brave.com' ],
            fingerprintingBlockedResources: [],
            httpsRedirectedResources: [ 'https://test.brave.com' ],
            javascriptBlockedResources: [ 'https://test.brave.com/index.js' ]
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
      assert.deepEqual(nextState, {
        currentWindowId: 1,
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
              'https://test.brave.com/': { actuallyBlocked: true, willBlock: true }
            },
            trackersBlockedResources: [ 'https://test.brave.com' ],
            adsBlockedResources: [ 'https://test.brave.com' ],
            fingerprintingBlockedResources: [ 'https://test.brave.com' ],
            httpsRedirectedResources: [ 'https://test.brave.com' ],
            javascriptBlockedResources: [ 'https://test.brave.com/index.js' ]
          }
        },
        windows: {
          1: 2
        }
      })
    })
  })

  describe('BLOCK_ADS_TRACKERS', function () {
    before(function () {
      this.reloadTabSpy = sinon.spy(tabsAPI, 'reloadTab')
      this.setAllowAdsSpy = sinon.spy(shieldsAPI, 'setAllowAds')
      this.setAllowTrackersSpy = sinon.spy(shieldsAPI, 'setAllowTrackers')
    })
    after(function () {
      this.reloadTabSpy.restore()
      this.setAllowAdsSpy.restore()
      this.setAllowTrackersSpy.restore()
    })
    it('should call setAllowAds and setAllowTrackers', function () {
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.BLOCK_ADS_TRACKERS,
          setting: 'allow'
        }), state)
      assert.equal(this.setAllowAdsSpy.withArgs(origin, 'block').calledOnce, true)
      assert.equal(this.setAllowTrackersSpy.withArgs(origin, 'block').calledOnce, true)
    })
  })

  describe('CONTROLS_TOGGLED', function () {
    before(function () {
      this.spy = sinon.spy(shieldsPanelState, 'updateTabShieldsData')
    })
    after(function () {
      this.spy.restore()
    })
    it('should call updateTabShieldsData', function () {
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.CONTROLS_TOGGLED,
          setting: true
        }), state)

      assert.equal(this.spy.calledOnce, true)
      assert.equal(this.spy.getCall(0).args[2].controlsOpen, true)
    })
  })

  describe('ALLOW_SCRIPT_ORIGINS_ONCE', function () {
    before(function () {
      this.reloadTabSpy = sinon.spy(tabsAPI, 'reloadTab')
      this.setAllowScriptOriginsOnceSpy = sinon.spy(shieldsAPI, 'setAllowScriptOriginsOnce')
    })
    after(function () {
      this.reloadTabSpy.restore()
      this.setAllowScriptOriginsOnceSpy.restore()
    })
    it('should call setAllowScriptOriginsOnce', function () {
      const origins = ['https://a.com/', 'https://b.com/']
      const tabId = 2
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.ALLOW_SCRIPT_ORIGINS_ONCE,
          origins
        }), state)
      assert.equal(this.setAllowScriptOriginsOnceSpy.withArgs(origins, tabId).calledOnce, true)
    })
  })

  describe('CHANGE_NO_SCRIPT_SETTINGS', function () {
    before(function () {
      this.spy = sinon.spy(shieldsPanelState, 'changeNoScriptSettings')
    })
    after(function () {
      this.spy.restore()
    })
    it('should call changeNoScriptSettings', function () {
      const tabId = 2
      const stateWithNoScriptInfo: State = {
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
            url: 'https://brave.com',
            noScriptInfo: {
              'https://brave.com': {
                actuallyBlocked: true,
                willBlock: true
              }
            },
            adsBlockedResources: [],
            trackersBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            fingerprintingBlockedResources: []
          }
        },
        windows: {
          1: 2
        },
        currentWindowId: 1
      }
      let nextState = shieldsPanelReducer(stateWithNoScriptInfo, {
        type: types.CHANGE_NO_SCRIPT_SETTINGS,
        origin
      })
      assert.deepEqual(nextState, {
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
            url: 'https://brave.com',
            noScriptInfo: {
              'https://brave.com': {
                actuallyBlocked: true,
                willBlock: false
              }
            },
            adsBlockedResources: [],
            trackersBlockedResources: [],
            httpsRedirectedResources: [],
            javascriptBlockedResources: [],
            fingerprintingBlockedResources: []
          }
        },
        windows: {
          1: 2
        },
        currentWindowId: 1
      })
      assert.equal(this.spy.withArgs(stateWithNoScriptInfo, tabId, origin).calledOnce, true)
    })
  })
})
