/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as assert from 'assert'
import * as types from '../../constants/webNavigationTypes'
import * as actions from '../../actions/webNavigationActions'

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
  it('onCommitted', () => {
    const tabId = 1
    const url = 'https://www.brave.com'
    const isMainFrame = true
    assert.deepEqual(actions.onCommitted(tabId, url, isMainFrame), {
      type: types.ON_COMMITTED,
      url,
      tabId,
      isMainFrame
    })
  })

})
