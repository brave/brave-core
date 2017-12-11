/* global describe, it, before, after */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as sinon from 'sinon'
import * as assert from 'assert'
import * as localeAPI from '../../../../app/background/api/localeAPI'

describe('locale API', () => {
  describe('getMessage', function () {
    before(function () {
      this.spy = sinon.spy(chrome.i18n, 'getMessage')
      this.message = 'NESPRESS YOURSELF'
      localeAPI.getMessage(this.message)
    })
    after(function () {
      this.spy.restore()
    })
    it('calls chrome.i18n.getMessage with the message', function () {
      assert(this.spy.calledOnce)
      assert.deepEqual(this.spy.getCall(0).args[0], this.message)
    })
  })
})
