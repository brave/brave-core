/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as types from '../../../app/constants/windowTypes'
import * as actions from '../../../app/actions/windowActions'

describe('windowActions', () => {
  it('windowFocusChanged', () => {
    const windowId = 1
    expect(actions.windowFocusChanged(windowId)).toEqual({
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
    expect(actions.windowCreated(win)).toEqual({
      type: types.WINDOW_CREATED,
      window: win
    })
  })

  it('windowRemoved', () => {
    const windowId = 1
    expect(actions.windowRemoved(windowId)).toEqual({
      type: types.WINDOW_REMOVED,
      windowId
    })
  })
})
