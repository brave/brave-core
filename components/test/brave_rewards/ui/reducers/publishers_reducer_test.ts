/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

import reducers from '../../../../brave_rewards/resources/ui/reducers/index'
import { types } from '../../../../brave_rewards/resources/ui/constants/rewards_types'
import { defaultState } from '../../../../brave_rewards/resources/ui/storage'
import { rewardsInitialState } from '../../../testData'

describe('publishers reducer', () => {
  describe('ON_EXCLUDE_PUBLISHER', () => {
    it('exclude is not defined in the state', () => {
      let result = reducers({
        rewardsData: { enabledMain: true }
      }, {
        type: types.ON_EXCLUDE_PUBLISHER,
        payload: {
          publisherKey: 'clifton.io'
        }
      })

      const expectedState: Rewards.State = {
        enabledMain: true,
        excluded: [
          'clifton.io'
        ]
      }

      expect(result).toEqual({
        rewardsData: expectedState
      })
    })

    it('exclude already has some data', () => {
      let result = reducers({
        rewardsData: { ...defaultState, excluded: ['clifton.io'] }
      }, {
        type: types.ON_EXCLUDE_PUBLISHER,
        payload: {
          publisherKey: 'brave.com'
        }
      })

      const expectedState: Rewards.State = {
        ...defaultState,
        excluded: [
          'clifton.io',
          'brave.com'
        ]
      }

      expect(result).toEqual({
        rewardsData: expectedState
      })
    })
  })
})
