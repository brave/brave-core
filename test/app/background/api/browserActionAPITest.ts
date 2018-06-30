/* global describe, it, before, after */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as sinon from 'sinon'
import * as assert from 'assert'
import * as browserActionAPI from '../../../../app/background/api/browserActionAPI'

describe('BrowserAction API', () => {
  describe('setBadgeText', function () {
    before(function () {
      this.spy = sinon.spy(chrome.browserAction, 'setBadgeText')
      this.text = '42'
      browserActionAPI.setBadgeText(this.text)
    })
    after(function () {
      this.spy.restore()
    })
    it('calls chrome.browserAction.setBadgeText with the text', function () {
      assert(this.spy.calledOnce)
      assert.deepEqual(this.spy.getCall(0).args[0], {
        text: this.text
      })
    })
  })
  describe('setIcon', function () {
    const enabledIconPath = 'img/icon-16.png'
    const disabledIconPath = 'img/icon-16-disabled.png'
    before(function () {
      this.spy = sinon.spy(chrome.browserAction, 'setIcon')
      this.url = 'https://brave.com'
      this.tabId = 1
      this.shieldsEnabled = true
    })
    after(function () {
      this.spy.restore()
    })
    afterEach(function () {
      this.spy.reset()
    })
    it('sets the enabled icon when protocol is http', function () {
      this.url = 'http://not-very-awesome-http-page.com'
      browserActionAPI.setIcon(this.url, this.tabId, this.shieldsEnabled)
      assert.deepEqual(this.spy.getCall(0).args[0], {
        path: enabledIconPath,
        tabId: this.tabId
      })
    })
    it('sets the enabled icon when protocol is https', function () {
      this.url = 'https://very-awesome-https-page.com'
      browserActionAPI.setIcon(this.url, this.tabId, this.shieldsEnabled)
      assert.deepEqual(this.spy.getCall(0).args[0], {
        path: enabledIconPath,
        tabId: this.tabId
      })
    })
    it('sets the disabled icon when the protocol is neither https nor http', function () {
      this.url = 'brave://welcome'
      browserActionAPI.setIcon(this.url, this.tabId, this.shieldsEnabled)
      assert.deepEqual(this.spy.getCall(0).args[0], {
        path: disabledIconPath,
        tabId: this.tabId
      })
    })
    it('sets the disabled icon when the protocol is http and shield is off', function () {
      this.url = 'http://not-very-awesome-http-page.com'
      this.shieldsEnabled = false
      browserActionAPI.setIcon(this.url, this.tabId, this.shieldsEnabled)
      assert.deepEqual(this.spy.getCall(0).args[0], {
        path: disabledIconPath,
        tabId: this.tabId
      })
    })
    it('sets the disabled icon when the protocol is https and shield is off', function () {
      this.url = 'https://very-awesome-https-page.com'
      this.shieldsEnabled = false
      browserActionAPI.setIcon(this.url, this.tabId, this.shieldsEnabled)
      assert.deepEqual(this.spy.getCall(0).args[0], {
        path: disabledIconPath,
        tabId: this.tabId
      })
    })
  })
})
