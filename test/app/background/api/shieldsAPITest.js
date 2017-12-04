/* global describe, it, before, after */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import sinon from 'sinon'
import assert from 'assert'
import actions from '../../../../app/background/actions/shieldsPanelActions'
import * as shieldsAPI from '../../../../app/background/api/shieldsAPI'
import { activeTabData, tabs } from '../../../testData'

describe('Shields API', () => {
  describe('getShieldSettingsForTabData', function () {
    it('returns a rejected promise when no tab data is specified', function (cb) {
      shieldsAPI.getShieldSettingsForTabData(undefined)
        .catch(() => {
          cb()
        })
    })
    it('resolves the returned promise with shield settings for the tab data', function (cb) {
      shieldsAPI.getShieldSettingsForTabData({
        url: 'https://www.brave.com/serg/dont/know/pokemon',
        origin: 'https://www.brave.com',
        hostname: 'www.brave.com',
        id: 5
      }).then((data) => {
        assert.deepEqual(data, {
          url: 'https://www.brave.com/serg/dont/know/pokemon',
          origin: 'https://www.brave.com',
          hostname: 'www.brave.com',
          adBlock: 'block',
          trackingProtection: 'block',
          httpsEverywhere: 'block',
          javascript: 'block',
          id: 5
        })
        cb()
      })
      .catch((e) => {
        console.error(e)
      })
    })
  })

  describe('getTabData', function () {
    before(function () {
      this.tabId = 2
      this.spy = sinon.spy(chrome.tabs, 'getAsync')
      this.p = shieldsAPI.getTabData(this.tabId)
    })
    after(function () {
      this.spy.restore()
    })
    it('calls chrome.tabs.getAsync for the ative tab', function () {
      assert(this.spy.withArgs(this.tabId).calledOnce)
    })
    it('resolves the promise with an array', function (cb) {
      this.p
        .then((tab) => {
          assert.deepEqual(tab, activeTabData)
          cb()
        })
        .catch((e) => {
          console.error(e)
        })
    })
  })

  describe('requestShieldPanelData', function () {
    before(function () {
      this.tabId = 2
      this.spy = sinon.stub(actions, 'shieldsPanelDataUpdated')
      this.p = shieldsAPI.requestShieldPanelData(this.tabId)
    })
    it('resolves and calls requestShieldPanelData', function (cb) {
      this.p
        .then(() => {
          assert(this.spy.calledOnce)
          assert.deepEqual(this.spy.getCall(0).args[0], tabs[this.tabId])
          cb()
        })
        .catch((e) => {
          console.error(e)
        })
    })
  })

  describe('setAllowAdBlock', function () {
    before(function () {
      this.spy = sinon.spy(chrome.contentSettings.braveAdBlock, 'setAsync')
      this.p = shieldsAPI.setAllowAdBlock('https://www.brave.com', 'block')
    })
    after(function () {
      this.spy.restore()
    })
    it('calls chrome.contentSettings.braveAdBlock with the correct args', function () {
      const arg0 = this.spy.getCall(0).args[0]
      assert.deepEqual(arg0, {
        primaryPattern: 'https://www.brave.com/*',
        setting: 'block'
      })
    })
    it('passes only 1 arg to chrome.contentSettings.braveAdBlock', function () {
      assert(this.spy.getCall(0).args.length, 1)
    })
    it('resolves the returned promise', function (cb) {
      this.p
        .then(cb)
        .catch((e) => {
          console.error(e)
        })
    })
  })

  describe('setAllowTrackingProtection', function () {
    before(function () {
      this.spy = sinon.spy(chrome.contentSettings.braveTrackingProtection, 'setAsync')
      this.p = shieldsAPI.setAllowTrackingProtection('https://www.brave.com', 'block')
    })
    after(function () {
      this.spy.restore()
    })
    it('calls chrome.contentSettings.braveTrackingProtection with the correct args', function () {
      const arg0 = this.spy.getCall(0).args[0]
      assert.deepEqual(arg0, {
        primaryPattern: 'https://www.brave.com/*',
        setting: 'block'
      })
    })
    it('passes only 1 arg to chrome.contentSettings.braveTrackingProtection', function () {
      assert(this.spy.getCall(0).args.length, 1)
    })
    it('resolves the returned promise', function (cb) {
      this.p
        .then(cb)
        .catch((e) => {
          console.error(e)
        })
    })
  })

  describe('setAllowHTTPSEverywhere', function () {
    before(function () {
      this.spy = sinon.spy(chrome.contentSettings.braveHTTPSEverywhere, 'setAsync')
      this.p = shieldsAPI.setAllowHTTPSEverywhere('https://www.brave.com', 'block')
    })
    after(function () {
      this.spy.restore()
    })
    it('calls chrome.contentSettings.braveHTTPSEverywhere with the correct args', function () {
      const arg0 = this.spy.getCall(0).args[0]
      assert.deepEqual(arg0, {
        primaryPattern: '*://www.brave.com/*',
        setting: 'block'
      })
    })
    it('passes only 1 arg to chrome.contentSettings.braveHTTPSEverywhere', function () {
      assert(this.spy.getCall(0).args.length, 1)
    })
    it('resolves the returned promise', function (cb) {
      this.p
        .then(cb)
        .catch((e) => {
          console.error(e)
        })
    })
  })

  describe('setAllowJavaScript', function () {
    before(function () {
      this.spy = sinon.spy(chrome.contentSettings.javascript, 'setAsync')
      this.p = shieldsAPI.setAllowJavaScript('https://www.brave.com', 'block')
    })
    after(function () {
      this.spy.restore()
    })
    it('calls chrome.contentSettings.javascript with the correct args', function () {
      const arg0 = this.spy.getCall(0).args[0]
      assert.deepEqual(arg0, {
        primaryPattern: 'https://www.brave.com/*',
        setting: 'block'
      })
    })
    it('passes only 1 arg to chrome.contentSettings.javascript', function () {
      assert(this.spy.getCall(0).args.length, 1)
    })
    it('resolves the returned promise', function (cb) {
      this.p
        .then(cb)
        .catch((e) => {
          console.error(e)
        })
    })
  })

  describe('toggleShieldsValue', function () {
    it('toggles \'allow\' to \'block\'', function () {
      assert.equal(shieldsAPI.toggleShieldsValue('allow'), 'block')
    })
    it('toggles \'block\' to \'allow\'', function () {
      assert.equal(shieldsAPI.toggleShieldsValue('block'), 'allow')
    })
  })
})
