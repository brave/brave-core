/* global describe, it, before, after */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as sinon from 'sinon'
import * as assert from 'assert'
import actions from '../../../../app/background/actions/shieldsPanelActions'
import * as shieldsAPI from '../../../../app/background/api/shieldsAPI'
import { activeTabData } from '../../../testData'
import { Tab as TabType } from '../../../../app/types/state/shieldsPannelState'
import * as resourceIdentifiers from '../../../../app/constants/resourceIdentifiers'

describe('Shields API', () => {
  describe('getShieldSettingsForTabData', function () {
    it('returns a rejected promise when no tab data is specified', function (cb) {
      shieldsAPI.getShieldSettingsForTabData(undefined)
        .catch(() => {
          cb()
        })
    })

    it('resolves the returned promise with shield settings for the tab data', function (cb) {
      const tab: chrome.tabs.Tab = {
        url: 'https://www.brave.com/serg/dont/know/pokemon',
        id: 5,
        index: 1,
        pinned: false,
        highlighted: false,
        windowId: 1,
        active: true,
        incognito: false,
        selected: false
      }

      shieldsAPI.getShieldSettingsForTabData(tab).then((data) => {
        assert.deepEqual(data, {
          url: 'https://www.brave.com/serg/dont/know/pokemon',
          origin: 'https://www.brave.com',
          hostname: 'www.brave.com',
          ads: 'block',
          trackers: 'block',
          httpUpgradableResources: 'block',
          javascript: 'block',
          braveShields: 'block',
          fingerprinting: 'block',
          cookies: 'block',
          id: 5
        })
        cb()
      })
      .catch((e: Error) => {
        console.error(e.toString())
      })
    })

    it('returns `block` by default for braveShields when origin is either http or https', function (cb) {
      const tab: chrome.tabs.Tab = {
        url: 'https://www.brave.com/charizard/knows/serg',
        index: 1,
        pinned: false,
        highlighted: false,
        windowId: 1,
        active: true,
        incognito: false,
        selected: false,
        id: 1337
      }

      shieldsAPI.getShieldSettingsForTabData(tab).then((data) => {
        const assertion = 'braveShields' in data && data.braveShields === 'block'
        assert(assertion)
        cb()
      })
      .catch((e: Error) => {
        console.error(e.toString())
      })
    })

    it('returns `block` by default for braveShields when origin is not http or https', function (cb) {
      const tab: chrome.tabs.Tab = {
        url: 'ftp://www.brave.com/serg/dont/know/pikachu',
        index: 1,
        pinned: false,
        highlighted: false,
        windowId: 1,
        active: true,
        incognito: false,
        selected: false,
        id: 1337
      }

      shieldsAPI.getShieldSettingsForTabData(tab).then((data) => {
        const assertion = 'braveShields' in data && data.braveShields === 'block'
        assert(assertion)
        cb()
      })
      .catch((e: Error) => {
        console.error(e.toString())
      })
    })

    it('returns `block` by default for braveShields when origin is an about page', function (cb) {
      const tab: chrome.tabs.Tab = {
        url: 'chrome://welcome',
        index: 1,
        pinned: false,
        highlighted: false,
        windowId: 1,
        active: true,
        incognito: false,
        selected: false,
        id: 1337
      }

      shieldsAPI.getShieldSettingsForTabData(tab).then((data) => {
        const assertion = 'braveShields' in data && data.braveShields === 'block'
        assert(assertion)
        cb()
      })
      .catch((e: Error) => {
        console.error(e.toString())
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

    it('calls chrome.tabs.getAsync for the active tab', function () {
      assert(this.spy.withArgs(this.tabId).calledOnce)
    })

    it('resolves the promise with an array', function (cb) {
      this.p
        .then((tab: chrome.tabs.Tab) => {
          assert.deepEqual(tab, activeTabData)
          cb()
        })
        .catch((e: Error) => {
          console.error(e.toString())
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
      const tab: Partial<TabType> = {
        url: 'https://www.brave.com/test',
        origin: 'https://www.brave.com',
        hostname: 'www.brave.com',
        id: 2,
        braveShields: 'block',
        ads: 'block',
        trackers: 'block',
        httpUpgradableResources: 'block',
        javascript: 'block',
        fingerprinting: 'block',
        cookies: 'block'
      }

      this.p
        .then(() => {
          assert(this.spy.calledOnce)
          assert.deepEqual(this.spy.getCall(0).args[0], tab)
          cb()
        })
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })

  describe('setAllowAds', function () {
    before(function () {
      this.spy = sinon.spy(chrome.braveShields.plugins, 'setAsync')
      this.p = shieldsAPI.setAllowAds('https://www.brave.com', 'block')
    })
    after(function () {
      this.spy.restore()
    })
    it('calls chrome.braveShields.plugins with the correct args', function () {
      const arg0 = this.spy.getCall(0).args[0]
      assert.deepEqual(arg0, {
        primaryPattern: 'https://www.brave.com/*',
        resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_ADS },
        setting: 'block',
        scope: 'incognito_session_only'
      })
    })
    it('passes only 1 arg to chrome.braveShields.plugins', function () {
      assert.equal(this.spy.getCall(0).args.length, 1)
    })
    it('resolves the returned promise', function (cb) {
      this.p
        .then(cb)
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })

  describe('setAllowTrackers', function () {
    before(function () {
      this.spy = sinon.spy(chrome.braveShields.plugins, 'setAsync')
      this.p = shieldsAPI.setAllowTrackers('https://www.brave.com', 'block')
    })
    after(function () {
      this.spy.restore()
    })
    it('calls chrome.braveShields.plugins with the correct args', function () {
      const arg0 = this.spy.getCall(0).args[0]
      assert.deepEqual(arg0, {
        primaryPattern: 'https://www.brave.com/*',
        resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_TRACKERS },
        setting: 'block',
        scope: 'incognito_session_only'
      })
    })
    it('passes only 1 arg to chrome.braveShields.plugins', function () {
      assert.equal(this.spy.getCall(0).args.length, 1)
    })
    it('resolves the returned promise', function (cb) {
      this.p
        .then(cb)
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })

  describe('setAllowHTTPUpgradableResource', function () {
    before(function () {
      this.spy = sinon.spy(chrome.braveShields.plugins, 'setAsync')
      this.p = shieldsAPI.setAllowHTTPUpgradableResources('https://www.brave.com', 'block')
    })
    after(function () {
      this.spy.restore()
    })
    it('calls chrome.braveShields.plugins with the correct args', function () {
      const arg0 = this.spy.getCall(0).args[0]
      assert.deepEqual(arg0, {
        primaryPattern: '*://www.brave.com/*',
        resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_HTTP_UPGRADABLE_RESOURCES },
        setting: 'block',
        scope: 'incognito_session_only'
      })
    })
    it('passes only 1 arg to chrome.braveShields.plugins', function () {
      assert.equal(this.spy.getCall(0).args.length, 1)
    })
    it('resolves the returned promise', function (cb) {
      this.p
        .then(cb)
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })

  describe('setAllowJavaScript', function () {
    before(function () {
      this.spy = sinon.spy(chrome.braveShields.javascript, 'setAsync')
      this.p = shieldsAPI.setAllowJavaScript('https://www.brave.com', 'block')
    })

    after(function () {
      this.spy.restore()
    })

    it('calls chrome.braveShields.plugins with the correct args', function () {
      const arg0 = this.spy.getCall(0).args[0]
      assert.deepEqual(arg0, {
        primaryPattern: 'https://www.brave.com/*',
        setting: 'block',
        scope: 'incognito_session_only'
      })
    })

    it('passes only 1 arg to chrome.braveShields.plugins', function () {
      assert.equal(this.spy.getCall(0).args.length, 1)
    })

    it('resolves the returned promise', function (cb) {
      this.p
        .then(cb)
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })

  describe('setAllowFingerprinting', function () {
    before(function () {
      this.spy = sinon.spy(chrome.braveShields.plugins, 'setAsync')
      this.p = shieldsAPI.setAllowFingerprinting('https://www.brave.com', 'block')
    })
    after(function () {
      this.spy.restore()
    })
    it('calls chrome.braveShields.plugins with the correct args', function () {
      const arg0 = this.spy.getCall(0).args[0]
      assert.deepEqual(arg0, {
        primaryPattern: 'https://www.brave.com/*',
        resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_FINGERPRINTING },
        setting: 'block',
        scope: 'incognito_session_only'
      })
      const arg1 = this.spy.getCall(1).args[0]
      assert.deepEqual(arg1, {
        primaryPattern: 'https://www.brave.com/*',
        secondaryPattern: 'https://firstParty/*',
        resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_FINGERPRINTING },
        setting: 'block',
        scope: 'incognito_session_only'
      })
    })
    it('passes only 1 arg to chrome.braveShields.plugins', function () {
      assert.equal(this.spy.getCall(0).args.length, 1)
      assert.equal(this.spy.getCall(1).args.length, 1)
    })
    it('resolves the returned promise', function (cb) {
      this.p
        .then(function() {
          cb()
        })
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })

  describe('setAllowCookies', function () {
    before(function () {
      this.spy = sinon.spy(chrome.braveShields.plugins, 'setAsync')
      this.p = shieldsAPI.setAllowCookies('https://www.brave.com', 'block')
    })
    after(function () {
      this.spy.restore()
    })
    it('calls chrome.braveShields.plugins with the correct args', function () {
      const arg0 = this.spy.getCall(0).args[0]
      assert.deepEqual(arg0, {
        primaryPattern: 'https://www.brave.com/*',
        resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_REFERRERS },
        setting: 'block',
        scope: 'incognito_session_only'
      })
      const arg1 = this.spy.getCall(1).args[0]
      assert.deepEqual(arg1, {
        primaryPattern: 'https://www.brave.com/*',
        resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_COOKIES },
        setting: 'block',
        scope: 'incognito_session_only'
      })
      const arg2 = this.spy.getCall(2).args[0]
      assert.deepEqual(arg2, {
        primaryPattern: 'https://www.brave.com/*',
        secondaryPattern: 'https://firstParty/*',
        resourceIdentifier: { id: resourceIdentifiers.RESOURCE_IDENTIFIER_COOKIES },
        setting: 'block',
        scope: 'incognito_session_only'
      })
    })
    it('passes only 1 arg to chrome.braveShields.plugins', function () {
      assert.equal(this.spy.getCall(0).args.length, 1)
      assert.equal(this.spy.getCall(1).args.length, 1)
    })
    it('resolves the returned promise', function (cb) {
      this.p
      .then(function () {
        cb()
      })
        .catch((e: Error) => {
          console.error(e.toString())
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

  describe('setAllowScriptOriginsOnce', function () {
    before(function () {
      this.spy = sinon.spy(chrome.braveShields, 'allowScriptsOnce')
      this.p = shieldsAPI.setAllowScriptOriginsOnce(
        ['https://a.com/', 'https://b.com/'], 2)
    })
    after(function () {
      this.spy.restore()
    })
    it('calls chrome.braveShields.allowScriptsOnce with the correct args', function () {
      const arg0 = this.spy.getCall(0).args[0]
      assert.deepEqual(arg0, ['https://a.com/', 'https://b.com/'])
      const arg1 = this.spy.getCall(0).args[1]
      assert.deepEqual(arg1, 2)
    })
    it('passes 3 args to chrome.braveShields.allowScriptsOnce', function () {
      assert.equal(this.spy.getCall(0).args.length, 3) // include callback
    })
    it('resolves the returned promise', function (cb) {
      this.p
        .then(function () {
          cb()
        })
        .catch((e: Error) => {
          console.error(e.toString())
        })
    })
  })
})
