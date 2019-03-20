/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

import adblockReducer from '../../../brave_adblock_ui/reducers/adblock_reducer'
import * as actions from '../../../brave_adblock_ui/actions/adblock_actions'
import { types } from '../../../brave_adblock_ui/constants/adblock_types'

describe('adblockReducer', () => {
  it('should handle initial state', () => {
    const assertion = adblockReducer(undefined, actions.statsUpdated())
    expect(assertion).toEqual({
      settings: {
        customFilters: '',
        regionalLists: []
      },
      stats: {
        adsBlockedStat: NaN,
        numBlocked: 0,
        regionalAdBlockEnabled: NaN,
        regionalAdBlockTitle: undefined
      }
    })
  })

  describe('ADBLOCK_STATS_UPDATED', () => {
    it('stats updated', () => {
      const assertion = adblockReducer(undefined, {
        type: types.ADBLOCK_STATS_UPDATED,
        payload: undefined
      })
      expect(assertion).toEqual({
        settings: {
          customFilters: '',
          regionalLists: []
        },
        stats: {
          adsBlockedStat: NaN,
          numBlocked: 0,
          regionalAdBlockEnabled: NaN,
          regionalAdBlockTitle: undefined
        }
      })
    })
  })
})
