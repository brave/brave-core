/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { types } from '../../../brave_adblock_ui/constants/adblock_types'
import * as actions from '../../../brave_adblock_ui/actions/adblock_actions'

describe('adblock_actions', () => {
  it('statsUpdated', () => {
    expect(actions.statsUpdated()).toEqual({
      type: types.ADBLOCK_STATS_UPDATED,
      meta: undefined,
      payload: undefined
    })
  })
})
