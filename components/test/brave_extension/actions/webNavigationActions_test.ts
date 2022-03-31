/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../../../brave_extension/extension/brave_extension/constants/webNavigationTypes'
import * as actions from '../../../brave_extension/extension/brave_extension/actions/webNavigationActions'

describe('webNavigationActions', () => {
  it('onBeforeNavigate', () => {
    const tabId = 1
    const url = 'https://www.brave.com'
    const isMainFrame = true
    expect(actions.onBeforeNavigate(tabId, url, isMainFrame)).toEqual({
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
    expect(actions.onCommitted(tabId, url, isMainFrame)).toEqual({
      type: types.ON_COMMITTED,
      url,
      tabId,
      isMainFrame
    })
  })
})
