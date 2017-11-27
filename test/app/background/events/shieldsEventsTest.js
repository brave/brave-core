/* global describe, it, before, after */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import sinon from 'sinon'
import assert from 'assert'
import '../../../../app/background/events/shieldsEvents'
import actions from '../../../../app/background/actions/shieldsPanelActions'
import { blockedResource } from '../../../testData'

describe('shieldsEvents events', () => {
  describe('chrome.braveShields.onBlocked listener', function () {
    before(function () {
      this.stub = sinon.stub(actions, 'resourceBlocked')
    })
    after(function () {
      this.stub.restore()
    })
    it('forward details to actions.resourceBlocked', function (cb) {
      chrome.braveShields.onBlocked.addListener((details) => {
        assert.equal(details, blockedResource)
        assert(this.stub.withArgs(details).calledOnce)
        cb()
      })
      chrome.braveShields.onBlocked.emit(blockedResource)
    })
  })
})
