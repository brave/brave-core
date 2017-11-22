/* global describe, it, before, after */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import sinon from 'sinon'
import assert from 'assert'
import '../../../../app/background/events/tabsEvents'
import * as shieldsAPI from '../../../../app/background/api/shieldsAPI'

describe('tabsEvents events', () => {
  describe('chrome.tabs.onActivated', function () {
    before(function () {
      this.spy = sinon.spy(shieldsAPI, 'updateShieldsSettings')
    })
    after(function () {
      this.spy.restore()
    })
    it('calls updateShieldsSettings', function (cb) {
      chrome.tabs.onActivated.addListener(() => {
        assert.equal(this.spy.calledOnce, true)
        cb()
      })
      chrome.tabs.onActivated.emit()
    })
  })
  describe('chrome.tabs.onUpdated', function () {
    before(function () {
      this.spy = sinon.spy(shieldsAPI, 'updateShieldsSettings')
    })
    after(function () {
      this.spy.restore()
    })
    it('calls updateShieldsSettings', function (cb) {
      chrome.tabs.onUpdated.addListener(() => {
        assert.equal(this.spy.calledOnce, true)
        cb()
      })
      chrome.tabs.onUpdated.emit()
    })
  })
})
