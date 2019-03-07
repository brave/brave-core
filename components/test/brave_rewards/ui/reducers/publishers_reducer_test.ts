/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

import reducers from '../../../../brave_rewards/resources/ui/reducers/index'
import { types } from '../../../../brave_rewards/resources/ui/constants/rewards_types'
import { defaultState } from '../../../../brave_rewards/resources/ui/storage'

describe('publishers reducer', () => {
  describe('ON_EXCLUDED_PUBLISHERS_NUMBER', () => {
    it('number is undefined', () => {
      const assertion = reducers(undefined, {
        type: types.ON_EXCLUDED_PUBLISHERS_NUMBER,
        payload: {
          num: undefined
        }
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.excludedPublishersNumber = 0

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })

    it('number is saved', () => {
      const assertion = reducers(undefined, {
        type: types.ON_EXCLUDED_PUBLISHERS_NUMBER,
        payload: {
          num: 1
        }
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.excludedPublishersNumber = 1

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })
})
