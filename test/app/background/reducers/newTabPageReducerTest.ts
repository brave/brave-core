/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as assert from 'assert'
import newTabPageReducer from '../../../../app/background/reducers/newTabPageReducer'
import * as actions from '../../../../app/actions/shieldsPanelActions'

describe('newTabPageReducer', () => {
  it('should handle initial state', () => {
    assert.deepEqual(
      newTabPageReducer(undefined, actions.blockAdsTrackers('allow'))
    , {})
  })

  describe('SETTINGS_ICON_CLICKED', function () {
    it('opens the settings page', function () {
    })
  })
})
