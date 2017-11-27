/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// import sinon from 'sinon'
import assert from 'assert'
// import * as types from '../../../../app/constants/newTabPageTypes'
import newTabPageReducer from '../../../../app/background/reducers/newTabPageReducer'
// import deepFreeze from 'deep-freeze-node'

describe('newTabPageReducer', () => {
  it('should handle initial state', () => {
    assert.deepEqual(
      newTabPageReducer(undefined, {})
    , {})
  })

  describe('SETTINGS_ICON_CLICKED', function () {
    it('opens the settings page', function () {
    })
  })
})
