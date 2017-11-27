/* global describe, it, before, after, afterEach */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import sinon from 'sinon'
import assert from 'assert'
import * as tabsAPI from '../../../../app/background/api/tabsAPI'

describe('tabs API', () => {
  describe('createTab', function () {
    before(function () {
      this.spy = sinon.spy(chrome.tabs, 'create')
      this.createProperties = { url: 'https://www.brave.com' }
      this.p = tabsAPI.createTab(this.createProperties)
    })
    after(function () {
      this.spy.restore()
    })
    it('calls chrome.tabs.create with the createProperties', function (cb) {
      this.p.then(() => {
        assert(this.spy.calledOnce)
        assert.deepEqual(this.spy.getCall(0).args[0], this.createProperties)
        cb()
      })
    })
  })
  describe('reloadTab', function () {
    before(function () {
      this.spy = sinon.spy(chrome.tabs, 'reload')
    })
    after(function () {
      this.spy.restore()
    })
    afterEach(function () {
      this.spy.reset()
    })
    it('calls chrome.tabs.reload without bypassing the cache', function (cb) {
      const tabId = 42
      const bypassCache = false
      const p = tabsAPI.reloadTab(tabId, bypassCache)
      p.then(() => {
        assert(this.spy.calledOnce)
        assert.equal(this.spy.getCall(0).args[0], tabId)
        assert.deepEqual(this.spy.getCall(0).args[1], {bypassCache})
        cb()
      })
    })
    it('calls chrome.tabs.reload with bypassing the cache', function (cb) {
      const tabId = 42
      const bypassCache = true
      const p = tabsAPI.reloadTab(tabId, bypassCache)
      p.then(() => {
        assert(this.spy.calledOnce)
        assert.equal(this.spy.getCall(0).args[0], tabId)
        assert.deepEqual(this.spy.getCall(0).args[1], {bypassCache})
        cb()
      })
    })
  })
})
