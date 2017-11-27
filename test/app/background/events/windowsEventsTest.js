/* global describe, it, before, after */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import sinon from 'sinon'
import assert from 'assert'
import '../../../../app/background/events/windowsEvents'
import actions from '../../../../app/background/actions/windowActions'

describe('windowsEvents events', () => {
  describe('chrome.windows.onFocusChanged', function () {
    before(function () {
      this.stub = sinon.stub(actions, 'windowFocusChanged')
    })
    after(function () {
      this.stub.restore()
    })
    it('calls actions.windowFocusChanged', function (cb) {
      const inputWindowId = 1
      chrome.windows.onFocusChanged.addListener((windowId) => {
        assert.equal(windowId, inputWindowId)
        assert.equal(this.stub.withArgs(inputWindowId).calledOnce, true)
        cb()
      })
      chrome.windows.onFocusChanged.emit(inputWindowId)
    })
  })
  describe('chrome.windows.onCreated', function () {
    const inputWindowId = 1
    before(function () {
      this.stub = sinon.stub(actions, 'windowCreated')
    })
    after(function () {
      this.stub.restore()
    })
    it('calls windowCreated', function (cb) {
      chrome.windows.onCreated.addListener((windowId) => {
        assert.equal(windowId, inputWindowId)
        assert.equal(this.stub.withArgs(inputWindowId).calledOnce, true)
        cb()
      })
      chrome.windows.onCreated.emit(inputWindowId)
    })
  })

  describe('chrome.windows.onRemoved', function () {
    before(function () {
      this.stub = sinon.stub(actions, 'windowRemoved')
    })
    after(function () {
      this.stub.restore()
    })
    it('calls updateShieldsSettings', function (cb) {
      const inputWindowId = 1
      chrome.windows.onRemoved.addListener((windowId) => {
        assert.equal(windowId, inputWindowId)
        assert.equal(this.stub.withArgs(inputWindowId).calledOnce, true)
        cb()
      })
      chrome.windows.onRemoved.emit(inputWindowId)
    })
  })
})
