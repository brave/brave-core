/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as assert from 'assert'
import * as types from '../../../app/constants/windowTypes'
import * as actions from '../../../app/actions/windowActions'

describe('windowActions', () => {
  it('windowFocusChanged', () => {
    const windowId = 1
    assert.deepEqual(actions.windowFocusChanged(windowId), {
      type: types.WINDOW_FOCUS_CHANGED,
      windowId
    })
  })

  it('windowCreated', () => {
    const win: chrome.windows.Window = {
      id: 1,
      state: 'normal',
      focused: true,
      alwaysOnTop: false,
      incognito: false,
      type: 'normal'
    }
    assert.deepEqual(actions.windowCreated(win), {
      type: types.WINDOW_CREATED,
      window: win
    })
  })

  it('windowRemoved', () => {
    const windowId = 1
    assert.deepEqual(actions.windowRemoved(windowId), {
      type: types.WINDOW_REMOVED,
      windowId
    })
  })
})
