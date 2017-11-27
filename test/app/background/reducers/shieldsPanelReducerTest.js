/* global describe, it, before, after, afterEach */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import sinon from 'sinon'
import assert from 'assert'
import * as types from '../../../../app/constants/shieldsPanelTypes'
import * as windowTypes from '../../../../app/constants/windowTypes'
import * as tabTypes from '../../../../app/constants/tabTypes'
import shieldsPanelReducer from '../../../../app/background/reducers/shieldsPanelReducer'
import * as shieldsAPI from '../../../../app/background/api/shieldsAPI'
import * as tabsAPI from '../../../../app/background/api/tabsAPI'
import * as shieldsPanelState from '../../../../app/state/shieldsPanelState'
import {initialState} from '../../../testData'

describe('braveShieldsPanelReducer', () => {
  it('should handle initial state', () => {
    assert.deepEqual(
      shieldsPanelReducer(undefined, {})
    , initialState.shieldsPanel)
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
      const state = {...initialState.shieldsPanel, windows: {1: this.tabId}, tabs: {}}
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
      this.state = {...initialState.shieldsPanel, windows: {1: this.tabId}, tabs: {}}
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
        tab: {
          active: true,
          id: this.tabId,
          windowId: this.windowId
        }
      })
      assert.equal(this.updateActiveTabSpy.calledOnce, true)
      assert.equal(this.updateActiveTabSpy.getCall(0).args[1], this.windowId)
      assert.equal(this.updateActiveTabSpy.getCall(0).args[2], this.tabId)
    })
    it('does not call shieldsPanelState.updateActiveTab when the tab is not active', function () {
      shieldsPanelReducer(this.state, {
        type: tabTypes.TAB_DATA_CHANGED,
        tab: {
          active: false,
          id: this.tabId,
          windowId: this.windowId
        }
      })
      assert.equal(this.updateActiveTabSpy.notCalled, true)
    })
  })

  describe('TAB_CREATED', function () {
    before(function () {
      this.updateActiveTabSpy = sinon.spy(shieldsPanelState, 'updateActiveTab')
      this.windowId = 1
      this.tabId = 2
      this.state = {...initialState.shieldsPanel, windows: {1: this.tabId}, tabs: {}}
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
          windowId: this.windowId
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
          windowId: this.windowId
        }
      })
      assert.equal(this.updateActiveTabSpy.notCalled, true)
    })
  })

  describe('SHIELDS_PANEL_DATA_UPDATED', function () {
    it('updates state detail', function () {
      const tabId = 2
      const details = {
        id: tabId,
        adBlock: 'block',
        trackingProtection: 'block'
      }
      assert.deepEqual(
        shieldsPanelReducer(initialState.shieldsPanel, {
          type: types.SHIELDS_PANEL_DATA_UPDATED,
          details
        }), {
          tabs: {
            [tabId]: {
              adsBlocked: 0,
              trackingProtectionBlocked: 0,
              id: tabId,
              adBlock: 'block',
              trackingProtection: 'block'
            }
          },
          windows: {}
        })
    })
  })

  const origin = 'https://brave.com'
  const state = {
    tabs: {
      2: {
        origin,
        hostname: 'brave.com'
      }
    },
    windows: {1: 2},
    currentWindowId: 1
  }

  describe('SHIELDS_TOGGLED', function () {
    before(function () {
      this.reloadTabSpy = sinon.spy(tabsAPI, 'reloadTab')
      this.setAllowAdBlockSpy = sinon.spy(shieldsAPI, 'setAllowAdBlock')
      this.setAllowTrackingProtectionSpy = sinon.spy(shieldsAPI, 'setAllowTrackingProtection')
    })
    after(function () {
      this.reloadTabSpy.restore()
      this.setAllowAdBlockSpy.restore()
      this.setAllowTrackingProtectionSpy.restore()
    })
    it('should call setAllowAdBlock and setAllowTrackingProtection', function () {
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.SHIELDS_TOGGLED
        }), state)
      assert.equal(this.setAllowAdBlockSpy.withArgs(origin, 'allow').calledOnce, true)
      assert.equal(this.setAllowTrackingProtectionSpy.withArgs(origin, 'allow').calledOnce, true)
    })
  })

  describe('AD_BLOCK_TOGGLED', function () {
    before(function () {
      this.reloadTabSpy = sinon.spy(tabsAPI, 'reloadTab')
      this.setAllowAdBlockSpy = sinon.spy(shieldsAPI, 'setAllowAdBlock')
    })
    after(function () {
      this.reloadTabSpy.restore()
      this.setAllowAdBlockSpy.restore()
    })
    it('should call setAllowAdBlock', function () {
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.AD_BLOCK_TOGGLED
        }), state)
      assert.equal(this.setAllowAdBlockSpy.withArgs(origin, 'allow').calledOnce, true)
    })
  })

  describe('TRACKING_PROTECTION_TOGGLED', function () {
    before(function () {
      this.reloadTabSpy = sinon.spy(tabsAPI, 'reloadTab')
      this.setAllowTrackingProtectionSpy = sinon.spy(shieldsAPI, 'setAllowTrackingProtection')
    })
    after(function () {
      this.reloadTabSpy.restore()
      this.setAllowTrackingProtectionSpy.restore()
    })
    it('should call setAllowTrackingProtection', function () {
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.TRACKING_PROTECTION_TOGGLED
        }), state)
      assert.equal(this.setAllowTrackingProtectionSpy.withArgs(origin, 'allow').calledOnce, true)
    })
  })

  describe('RESOURCE_BLOCKED', function () {
    it('increases same count consecutively', function () {
      let nextState = shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'adBlock',
          tabId: 2
        }
      })
      assert.deepEqual(nextState, {
        currentWindowId: 1,
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 1,
            trackingProtectionBlocked: 0
          }
        },
        windows: {1: 2}
      })

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'adBlock',
          tabId: 2
        }
      })
      assert.deepEqual(nextState, {
        currentWindowId: 1,
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 2,
            trackingProtectionBlocked: 0
          }
        },
        windows: {1: 2}
      })
    })
    it('increases different tab counts separately', function () {
      let nextState = shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'adBlock',
          tabId: 2
        }
      })
      assert.deepEqual(nextState, {
        currentWindowId: 1,
        tabs: {
          2: {
            adsBlocked: 1,
            trackingProtectionBlocked: 0,
            origin: 'https://brave.com',
            hostname: 'brave.com'
          }
        },
        windows: {1: 2}
      })

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'adBlock',
          tabId: 3
        }
      })
      assert.deepEqual(nextState, {
        currentWindowId: 1,
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 1,
            trackingProtectionBlocked: 0
          },
          3: {
            adsBlocked: 1,
            trackingProtectionBlocked: 0
          }
        },
        windows: {1: 2}
      })
    })
    it('increases different resource types separately', function () {
      let nextState = shieldsPanelReducer(state, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'adBlock',
          tabId: 2
        }
      })
      assert.deepEqual(nextState, {
        currentWindowId: 1,
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 1,
            trackingProtectionBlocked: 0
          }
        },
        windows: {1: 2}
      })

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'trackingProtection',
          tabId: 2
        }
      })
      assert.deepEqual(nextState, {
        currentWindowId: 1,
        tabs: {
          2: {
            origin: 'https://brave.com',
            hostname: 'brave.com',
            adsBlocked: 1,
            trackingProtectionBlocked: 1
          }
        },
        windows: {1: 2}
      })
    })
  })
})
