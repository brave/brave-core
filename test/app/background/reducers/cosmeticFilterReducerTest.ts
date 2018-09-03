/* global describe, it, before, after, afterEach */
/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as sinon from 'sinon'
import * as assert from 'assert'
import * as shieldPanelTypes from '../../../../app/constants/shieldsPanelTypes'
import * as cosmeticFilterTypes from '../../../../app/constants/cosmeticFilterTypes'
import * as windowTypes from '../../../../app/constants/windowTypes'
import * as tabTypes from '../../../../app/constants/tabTypes'
import * as webNavigationTypes from '../../../../app/constants/webNavigationTypes'
import shieldsPanelReducer from '../../../../app/background/reducers/shieldsPanelReducer'
import * as shieldsAPI from '../../../../app/background/api/shieldsAPI'
import cosmeticFilterReducer from '../../../../app/background/reducers/cosmeticFilterReducer'
import * as cosmeticFilterAPI from '../../../../app/background/api/cosmeticFilterAPI'
import * as tabsAPI from '../../../../app/background/api/tabsAPI'
import * as shieldsPanelState from '../../../../app/state/shieldsPanelState'
import { initialState } from '../../../testData'
import * as deepFreeze from 'deep-freeze-node'
import { ShieldDetails } from '../../../../app/types/actions/shieldsPanelActions'
import * as actions from '../../../../app/actions/shieldsPanelActions'
import { State } from '../../../../app/types/state/shieldsPannelState'

describe('cosmeticFilterReducer', () => {
  it('should handle initial state', () => {
    assert.deepEqual(
      shieldsPanelReducer(undefined, actions.blockAdsTrackers('allow')), initialState.cosmeticFilter)
  })
  describe('ON_COMMITTED', function () {
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
        type: webNavigationTypes.ON_COMMITTED,
        tabId: this.tabId,
        url: 'https://www.brave.com',
        isMainFrame: true
      })
      assert.equal(this.spy.calledOnce, true)
      assert.equal(this.spy.getCall(0).args[1], this.tabId)
    })
    it('does not call resetBlockingStats when isMainFrame is false', function () {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_COMMITTED,
        tabId: this.tabId,
        url: 'https://www.brave.com',
        isMainFrame: false
      })
      assert.equal(this.spy.notCalled, true)
    })
    it('calls resetNoScriptInfo when isMainFrame is true', function () {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_COMMITTED,
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
        type: webNavigationTypes.ON_COMMITTED,
        tabId: this.tabId,
        url: 'https://www.brave.com',
        isMainFrame: false
      })
      assert.equal(this.resetNoScriptInfoSpy.notCalled, true)
    })
    it('calls resetBlockingResources when isMainFrame is true', function () {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_COMMITTED,
        tabId: this.tabId,
        url: 'https://www.brave.com',
        isMainFrame: true
      })
      assert.equal(this.spy.calledOnce, true)
      assert.equal(this.spy.getCall(0).args[1], this.tabId)
    })
    it('does not call resetBlockingResources when isMainFrame is false', function () {
      shieldsPanelReducer(initialState.shieldsPanel, {
        type: webNavigationTypes.ON_COMMITTED,
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
            type: shieldPanelTypes.SHIELDS_PANEL_DATA_UPDATED,
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
          type: shieldPanelTypes.SHIELDS_TOGGLED,
          setting: 'allow'
        }), state)
      assert.equal(this.setAllowBraveShieldsSpy.withArgs(origin, 'allow').calledOnce, true)
    })
  })
  describe('SITE_COSMETIC_FILTER_REMOVED', function () {
    before(function () {
      this.removeSiteFilterSpy = sinon.spy(cosmeticFilterAPI, 'removeSiteFilter')
    })
    after(function () {
      this.removeSiteFilterSpy.restore()
    })
    it('should call removeSiteFilter', function () {
      assert.deepEqual(
              cosmeticFilterReducer(state, {
                type: cosmeticFilterTypes.SITE_COSMETIC_FILTER_REMOVED,
                origin
              }), state)
    })
  })
  describe('ALL_COSMETIC_FILTERS_REMOVED', function () {
    before(function () {
      this.removeSiteFilterSpy = sinon.spy(cosmeticFilterAPI, 'removeAllFilters')
    })
    after(function () {
      this.removeSiteFilterSpy.restore()
    })
    it('should call removeAllFilters', function () {
      assert.deepEqual(
        cosmeticFilterReducer(state, {
          type: cosmeticFilterTypes.ALL_COSMETIC_FILTERS_REMOVED
        }), state)
    })
  })
  describe('SITE_COSMETIC_FILTER_ADDED', function () {
    before(function () {
      this.removeSiteFilterSpy = sinon.spy(cosmeticFilterAPI, 'addSiteCosmeticFilter')
    })
    after(function () {
      this.removeSiteFilterSpy.restore()
    })
    it('should call addSiteCosmeticFilter', function () {
      const cssfilter = '#test'
      assert.deepEqual(
        cosmeticFilterReducer(state, {
          type: cosmeticFilterTypes.SITE_COSMETIC_FILTER_ADDED,
          origin,
          cssfilter
        }), state)
    })
  })
})
