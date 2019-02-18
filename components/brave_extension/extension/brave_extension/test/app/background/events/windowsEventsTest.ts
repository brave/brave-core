/* global describe, it, before, after */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as sinon from 'sinon'
import * as assert from 'assert'
import '../../../../app/background/events/windowsEvents'
import actions from '../../../../app/background/actions/windowActions'

interface WindowIdEvent extends chrome.events.Event<(windowId: number) => void> {
  emit: (windowId: number) => void
}

interface WindowReferenceEvent extends chrome.events.Event<(window: chrome.windows.Window) => void> {
  emit: (window: chrome.windows.Window) => void
}

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
      const onFocusChanged = chrome.windows.onFocusChanged as WindowIdEvent
      onFocusChanged.addListener((windowId) => {
        assert.equal(windowId, inputWindowId)
        assert.equal(this.stub.withArgs(inputWindowId).calledOnce, true)
        cb()
      })
      onFocusChanged.emit(inputWindowId)
    })
  })
  describe('chrome.windows.onCreated', function () {
    const window = {
      id: 1,
      state: 'normal',
      focused: true,
      alwaysOnTop: false,
      incognito: false,
      type: 'normal'
    }
    before(function () {
      this.stub = sinon.stub(actions, 'windowCreated')
    })
    after(function () {
      this.stub.restore()
    })
    it('calls windowCreated', function (cb) {
      const onCreated = chrome.windows.onCreated as WindowReferenceEvent

      onCreated.addListener((windowId) => {
        assert.equal(windowId, window)
        assert.equal(this.stub.withArgs(window).calledOnce, true)
        cb()
      })
      onCreated.emit(window)
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
      const onRemoved = chrome.windows.onRemoved as WindowIdEvent

      onRemoved.addListener((windowId) => {
        assert.equal(windowId, inputWindowId)
        assert.equal(this.stub.withArgs(inputWindowId).calledOnce, true)
        cb()
      })
      onRemoved.emit(inputWindowId)
    })
  })
})
