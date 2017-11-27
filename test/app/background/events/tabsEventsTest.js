/* global describe, it, before, after */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import sinon from 'sinon'
import assert from 'assert'
import '../../../../app/background/events/tabsEvents'
import actions from '../../../../app/background/actions/tabActions'

describe('tabsEvents events', () => {
  describe('chrome.tabs.onActivated', function () {
    before(function () {
      this.stub = sinon.stub(actions, 'activeTabChanged')
    })
    after(function () {
      this.stub.restore()
    })
    it('calls actions.activeTabChanged with the correct args', function (cb) {
      const inputWindowId = 1
      const inputTabId = 2
      chrome.tabs.onActivated.addListener((activeInfo) => {
        assert.equal(activeInfo.windowId, inputWindowId)
        assert.equal(activeInfo.tabId, inputTabId)
        assert.equal(this.stub.withArgs(inputWindowId, inputTabId).calledOnce, true)
        cb()
      })
      chrome.tabs.onActivated.emit({ windowId: inputWindowId, tabId: inputTabId })
    })
  })
  describe('chrome.tabs.onCreated', function () {
    const inputTab = { id: 3 }
    before(function () {
      this.stub = sinon.stub(actions, 'tabCreated')
    })
    after(function () {
      this.stub.restore()
    })
    it('calls tabCreated with the correct args', function (cb) {
      chrome.tabs.onCreated.addListener((tab) => {
        assert.equal(tab, inputTab)
        assert.equal(this.stub.withArgs(inputTab).calledOnce, true)
        cb()
      })
      chrome.tabs.onCreated.emit(inputTab)
    })
  })

  describe('chrome.tabs.onUpdated', function () {
    const inputTabId = 3
    const inputChangeInfo = {}
    const inputTab = {id: 3}
    before(function () {
      this.stub = sinon.stub(actions, 'tabDataChanged')
    })
    after(function () {
      this.stub.restore()
    })
    it('calls tabDataChanged', function (cb) {
      chrome.tabs.onUpdated.addListener((tabId, changeInfo, tab) => {
        assert.equal(tabId, inputTabId)
        assert.equal(changeInfo, inputChangeInfo)
        assert.equal(tab, inputTab)
        assert.equal(this.stub.withArgs(tabId, changeInfo, tab).calledOnce, true)
        cb()
      })
      chrome.tabs.onUpdated.emit(inputTabId, inputChangeInfo, inputTab)
    })
  })
})
