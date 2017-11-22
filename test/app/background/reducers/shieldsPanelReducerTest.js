/* global describe, it, before, after, afterEach */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import sinon from 'sinon'
import assert from 'assert'
import * as types from '../../../../app/constants/shieldsPanelTypes'
import shieldsPanelReducer from '../../../../app/background/reducers/shieldsPanelReducer'
import * as shieldsAPI from '../../../../app/background/api/shieldsAPI'

describe('braveShieldsPanelReducer', () => {
  it('should handle initial state', () => {
    assert.deepEqual(
      shieldsPanelReducer(undefined, {})
    , {tabs: {}})
  })

  before(function () {
    this.setAllowAdBlockSpy = sinon.spy(shieldsAPI, 'setAllowAdBlock')
    this.setAllowTrackingProtectionSpy = sinon.spy(shieldsAPI, 'setAllowTrackingProtection')
  })

  afterEach(function () {
    this.setAllowAdBlockSpy.reset()
    this.setAllowTrackingProtectionSpy.reset()
  })

  after(function () {
    this.setAllowAdBlockSpy.restore()
    this.setAllowTrackingProtectionSpy.restore()
  })

  describe('SHIELDS_PANEL_DATA_UPDATED', function () {
    it('updates state detail', function () {
      const state = {
        tabs: {},
        hostname: 'brave.com',
        url: 'https://brave.com',
        adBlock: 'allow',
        trackingProtection: 'allow'
      }
      const details = {
        hostname: 'brianbondy.com',
        url: 'https://brianbondy.com',
        adBlock: 'block',
        trackingProtection: 'block',
        tabs: {}
      }
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.SHIELDS_PANEL_DATA_UPDATED,
          details
        }), details)
      assert.equal(this.setAllowAdBlockSpy.notCalled, true)
      assert.equal(this.setAllowTrackingProtectionSpy.notCalled, true)
    })
  })

  const origin = 'https://brave.com'
  const state = {
    origin,
    hostname: 'brave.com',
    tabs: {}
  }

  describe('SHIELDS_TOGGLED', function () {
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
    it('should call setAllowAdBlock', function () {
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.AD_BLOCK_TOGGLED
        }), state)
      assert.equal(this.setAllowAdBlockSpy.withArgs(origin, 'allow').calledOnce, true)
      assert.equal(this.setAllowTrackingProtectionSpy.notCalled, true)
    })
  })

  describe('TRACKING_PROTECTION_TOGGLED', function () {
    it('should call setAllowTrackingProtection', function () {
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.TRACKING_PROTECTION_TOGGLED
        }), state)
      assert.equal(this.setAllowAdBlockSpy.notCalled, true)
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
        origin: 'https://brave.com',
        hostname: 'brave.com',
        tabs: {
          2: {
            adsBlocked: 1,
            trackingProtectionBlocked: 0
          }
        }
      })

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'adBlock',
          tabId: 2
        }
      })
      assert.deepEqual(nextState, {
        origin: 'https://brave.com',
        hostname: 'brave.com',
        tabs: {
          2: {
            adsBlocked: 2,
            trackingProtectionBlocked: 0
          }
        }
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
        origin: 'https://brave.com',
        hostname: 'brave.com',
        tabs: {
          2: {
            adsBlocked: 1,
            trackingProtectionBlocked: 0
          }
        }
      })

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'adBlock',
          tabId: 3
        }
      })
      assert.deepEqual(nextState, {
        origin: 'https://brave.com',
        hostname: 'brave.com',
        tabs: {
          2: {
            adsBlocked: 1,
            trackingProtectionBlocked: 0
          },
          3: {
            adsBlocked: 1,
            trackingProtectionBlocked: 0
          }
        }
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
        origin: 'https://brave.com',
        hostname: 'brave.com',
        tabs: {
          2: {
            adsBlocked: 1,
            trackingProtectionBlocked: 0
          }
        }
      })

      nextState = shieldsPanelReducer(nextState, {
        type: types.RESOURCE_BLOCKED,
        details: {
          blockType: 'trackingProtection',
          tabId: 2
        }
      })
      assert.deepEqual(nextState, {
        origin: 'https://brave.com',
        hostname: 'brave.com',
        tabs: {
          2: {
            adsBlocked: 1,
            trackingProtectionBlocked: 1
          }
        }
      })
    })
  })
})
