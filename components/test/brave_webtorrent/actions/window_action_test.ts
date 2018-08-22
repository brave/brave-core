/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../../brave_webtorrent/extension/constants/window_types'
import * as actions from '../../../brave_webtorrent/extension/actions/window_actions'

describe('window_actions', () => {
  it('windowCreated', () => {
    const window: chrome.windows.Window = {
      id: 1,
      state: 'normal',
      focused: true,
      alwaysOnTop: false,
      incognito: false,
      type: 'normal'
    }
    expect(actions.windowCreated(window)).toEqual({
      type: types.WINDOW_CREATED,
      meta: undefined,
      payload: { window }
    })
  })

  const windowId = 7

  it('windowRemoved', () => {
    expect(actions.windowRemoved(windowId)).toEqual({
      type: types.WINDOW_REMOVED,
      meta: undefined,
      payload: { windowId }
    })
  })

  it('windowFocusChanged', () => {
    expect(actions.windowFocusChanged(windowId)).toEqual({
      type: types.WINDOW_FOCUS_CHANGED,
      meta: undefined,
      payload: { windowId }
    })
  })
})
