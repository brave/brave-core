/* global describe, it, before, after, afterEach */

import * as types from '../../../app/constants/shieldsPanelTypes'
import shieldsPanelReducer from '../../../app/background/reducers/shieldsPanelReducer'
import sinon from 'sinon'
import * as shieldsAPI from '../../../app/background/api/shields'
import assert from 'assert'

describe('braveShieldsPanelReducer', () => {
  it('should handle initial state', () => {
    assert.deepEqual(
      shieldsPanelReducer(undefined, {})
    , {})
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
        hostname: 'brave.com',
        url: 'https://brave.com',
        adBlock: 'allow',
        trackingProtection: 'allow'
      }
      const details = {
        hostname: 'brianbondy.com',
        url: 'https://brianbondy.com',
        adBlock: 'block',
        trackingProtection: 'block'
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
    hostname: 'brave.com'
  }

  describe('TOGGLE_SHIELDS', function () {
    it('should call setAllowAdBlock and setAllowTrackingProtection', function () {
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.TOGGLE_SHIELDS
        }), state)
      assert.equal(this.setAllowAdBlockSpy.withArgs(origin, 'allow').calledOnce, true)
      assert.equal(this.setAllowTrackingProtectionSpy.withArgs(origin, 'allow').calledOnce, true)
    })
  })

  describe('TOGGLE_AD_BLOCK', function () {
    it('should call setAllowAdBlock', function () {
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.TOGGLE_AD_BLOCK
        }), state)
      assert.equal(this.setAllowAdBlockSpy.withArgs(origin, 'allow').calledOnce, true)
      assert.equal(this.setAllowTrackingProtectionSpy.notCalled, true)
    })
  })

  describe('TOGGLE_TRACKING_PROTECTION', function () {
    it('should call setAllowTrackingProtection', function () {
      assert.deepEqual(
        shieldsPanelReducer(state, {
          type: types.TOGGLE_TRACKING_PROTECTION
        }), state)
      assert.equal(this.setAllowAdBlockSpy.notCalled, true)
      assert.equal(this.setAllowTrackingProtectionSpy.withArgs(origin, 'allow').calledOnce, true)
    })
  })
})
