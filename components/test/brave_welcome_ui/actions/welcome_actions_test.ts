/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../../brave_welcome_ui/constants/welcome_types'
import * as actions from '../../../brave_welcome_ui/actions/welcome_actions'

describe('welcome_actions', () => {
  it('importNowRequested', () => {
    expect(actions.importNowRequested()).toEqual({
      type: types.IMPORT_NOW_REQUESTED,
      meta: undefined,
      payload: undefined
    })
  })

  it('goToPageRequested', () => {
    expect(actions.goToPageRequested(1337)).toEqual({
      type: types.GO_TO_PAGE_REQUESTED,
      meta: undefined,
      payload: { pageIndex: 1337 }
    })
  })
})