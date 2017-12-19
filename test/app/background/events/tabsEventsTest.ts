/* global describe, it, before, after */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as sinon from 'sinon'
import * as assert from 'assert'
import '../../../../app/background/events/tabsEvents'
import actions from '../../../../app/background/actions/tabActions'

interface TabActivatedEvent extends chrome.events.Event<(activeInfo: chrome.tabs.TabActiveInfo) => void> {
  emit: (detail: chrome.tabs.TabActiveInfo) => void
}

interface TabCreatedEvent extends chrome.events.Event<(activeInfo: chrome.tabs.Tab) => void> {
  emit: (detail: chrome.tabs.Tab) => void
}

interface TabUpdatedEvent extends chrome.events.Event<(tabId: number, changeInfo: chrome.tabs.TabChangeInfo, tab: chrome.tabs.Tab) => void> {
  emit: (tabId: number, changeInfo: chrome.tabs.TabChangeInfo, tab: chrome.tabs.Tab) => void
}

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

      const onActivated = chrome.tabs.onActivated as TabActivatedEvent

      onActivated.addListener((activeInfo) => {
        assert.equal(activeInfo.windowId, inputWindowId)
        assert.equal(activeInfo.tabId, inputTabId)
        assert.equal(this.stub.withArgs(inputWindowId, inputTabId).calledOnce, true)
        cb()
      })
      onActivated.emit({ windowId: inputWindowId, tabId: inputTabId })
    })
  })
  describe('chrome.tabs.onCreated', function () {
    const inputTab = {
      id: 3,
      index: 0,
      pinned: false,
      highlighted: false,
      windowId: 1,
      active: true,
      incognito: false,
      selected: true
    }
    before(function () {
      this.stub = sinon.stub(actions, 'tabCreated')
    })
    after(function () {
      this.stub.restore()
    })
    it('calls tabCreated with the correct args', function (cb) {

      const onCreated = chrome.tabs.onCreated as TabCreatedEvent

      onCreated.addListener((tab) => {
        assert.equal(tab, inputTab)
        assert.equal(this.stub.withArgs(inputTab).calledOnce, true)
        cb()
      })
      onCreated.emit(inputTab)
    })
  })

  describe('chrome.tabs.onUpdated', function () {
    const inputTabId = 3
    const inputChangeInfo = {}
    const inputTab = {
      id: 3,
      index: 0,
      pinned: false,
      highlighted: false,
      windowId: 1,
      active: true,
      incognito: false,
      selected: true
    }
    before(function () {
      this.stub = sinon.stub(actions, 'tabDataChanged')
    })
    after(function () {
      this.stub.restore()
    })
    it('calls tabDataChanged', function (cb) {
      const onUpdated = chrome.tabs.onUpdated as TabUpdatedEvent

      onUpdated.addListener((tabId, changeInfo, tab) => {
        assert.equal(tabId, inputTabId)
        assert.equal(changeInfo, inputChangeInfo)
        assert.equal(tab, inputTab)
        assert.equal(this.stub.withArgs(tabId, changeInfo, tab).calledOnce, true)
        cb()
      })
      onUpdated.emit(inputTabId, inputChangeInfo, inputTab)
    })
  })
})
