/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { createReducer } from '../../../../brave_rewards/resources/page/reducers'
import { types } from '../../../../brave_rewards/resources/page/actions/rewards_types'
import { defaultState } from '../../../../brave_rewards/resources/page/reducers/default_state'

describe('publishers reducer', () => {
  const reducers = createReducer()

  describe('ON_EXCLUDED_LIST', () => {
    it('updates list', () => {
      const assertion = reducers(undefined, {
        type: types.ON_EXCLUDED_LIST,
        payload: {
          list: [
            {
              id: 'foo.com',
              verified: false,
              url: 'https://foo.com',
              name: 'Foo Bar',
              provider: '',
              favicon: ''
            }
          ]
        }
      })

      const expectedState: Rewards.State = defaultState()
      expectedState.excludedList = [
        {
          id: 'foo.com',
          verified: false,
          url: 'https://foo.com',
          name: 'Foo Bar',
          provider: '',
          favicon: ''
        }
      ]

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })

    it('does not update on bad payload', () => {
      const initialState = defaultState()
      initialState.excludedList = [
        {
          id: 'foo.com',
          verified: false,
          url: 'https://foo.com',
          name: 'Foo Bar',
          provider: '',
          favicon: ''
        }
      ]

      const assertion = reducers({ rewardsData: initialState }, {
        type: types.ON_EXCLUDED_LIST,
        payload: {}
      })

      const expectedState: Rewards.State = defaultState()
      expectedState.excludedList = [
        {
          id: 'foo.com',
          verified: false,
          url: 'https://foo.com',
          name: 'Foo Bar',
          provider: '',
          favicon: ''
        }
      ]

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })
})
