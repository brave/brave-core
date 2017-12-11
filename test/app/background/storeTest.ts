/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as assert from 'assert'
import store from '../../../app/background/store'
import {initialState} from '../../testData'

describe('store test', () => {
  it('store can get state', function () {
    assert.deepEqual(store.getState(), initialState)
  })
})
