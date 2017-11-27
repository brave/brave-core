/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import assert from 'assert'
import * as types from '../../../app/constants/webNavigationTypes'
import * as actions from '../../../app/actions/webNavigationActions'

describe('webNavigationActions', () => {
  it('onBeforeNavigate', () => {
    const tabId = 1
    const url = 'https://www.brave.com'
    const isMainFrame = true
    assert.deepEqual(actions.onBeforeNavigate(tabId, url, isMainFrame), {
      type: types.ON_BEFORE_NAVIGATION,
      url,
      tabId,
      isMainFrame
    })
  })
})
