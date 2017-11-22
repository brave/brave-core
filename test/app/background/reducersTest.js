/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import assert from 'assert'
import reducers from '../../../app/background/reducers'
import {initialState} from '../../testData'

describe('reducers test', () => {
  it('reduers is a combined reducer function', function () {
    assert.equal(typeof reducers, 'function')
  })
  it('reduers passed with no state, gets initial state', function () {
    assert.deepEqual(reducers(undefined, {}), initialState)
  })
  it('reduers passed with an unknown action returns the same input', function () {
    const unknownAction = {
      type: 'DA07BE00-43FB-44C3-B020-49308A0D3E78'
    }
    const state = {...initialState, newTabPage: { a: 1 }}
    assert.equal(reducers(state, unknownAction), state)
  })
})
