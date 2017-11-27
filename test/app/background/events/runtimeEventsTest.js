/* global describe, it, before, after */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import sinon from 'sinon'
import assert from 'assert'
import '../../../../app/background/events/runtimeEvents'
import windowActions from '../../../../app/background/actions/windowActions'
import tabActions from '../../../../app/background/actions/tabActions'

describe('runtimeEvents events', () => {
  describe('chrome.runtime.onStartup listener', function () {
    const inputWindows = [{
      id: 1,
      tabs: [{
        id: 1
      }, {
        id: 2
      }]
    }, {
      id: 2,
      tabs: [{
        id: 3
      }]
    }]
    let deferred
    const p = new Promise((resolve, reject) => {
      deferred = resolve
    })
    before(function (cb) {
      this.windowCreatedStub = sinon.stub(windowActions, 'windowCreated')
      this.tabCreatedStub = sinon.stub(tabActions, 'tabCreated')
      this.windowGetAllStub = sinon.stub(chrome.windows, 'getAllAsync').callsFake(() => {
        deferred(inputWindows)
        return p
      })
      chrome.runtime.onStartup.addListener(cb)
      chrome.runtime.onStartup.emit()
    })
    after(function () {
      this.windowCreatedStub.restore()
      this.tabCreatedStub.restore()
      this.windowGetAllStub.restore()
    })
    it('calls windowActions.windowCreated for each window', function (cb) {
      p.then((inputWindows) => {
        assert.equal(this.windowCreatedStub.getCalls().length, 2)
        assert.equal(this.windowCreatedStub.getCall(0).args.length, 1)
        assert.equal(this.windowCreatedStub.getCall(0).args[0], inputWindows[0])
        assert.equal(this.windowCreatedStub.getCall(1).args.length, 1)
        assert.equal(this.windowCreatedStub.getCall(1).args[0], inputWindows[1])
        cb()
      })
    })
    it('calls tabActions.tabCreated for each tab in each window', function (cb) {
      p.then((inputWindows) => {
        assert.equal(this.tabCreatedStub.getCalls().length, 3)
        assert.equal(this.tabCreatedStub.getCall(0).args.length, 1)
        assert.equal(this.tabCreatedStub.getCall(0).args[0], inputWindows[0].tabs[0])
        assert.equal(this.tabCreatedStub.getCall(1).args[0], inputWindows[0].tabs[1])
        assert.equal(this.tabCreatedStub.getCall(2).args.length, 1)
        assert.equal(this.tabCreatedStub.getCall(2).args[0], inputWindows[1].tabs[0])
        cb()
      })
    })
  })
})
