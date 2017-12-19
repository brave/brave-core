/* global describe, it, before, after */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as sinon from 'sinon'
import * as assert from 'assert'
import * as badgeAPI from '../../../../app/background/api/badgeAPI'

describe('Badge API', () => {
  describe('setBadgeText', function () {
    before(function () {
      this.spy = sinon.spy(chrome.browserAction, 'setBadgeText')
      this.text = '42'
      badgeAPI.setBadgeText(this.text)
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
})
