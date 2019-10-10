/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

import reducers from '../../../../brave_rewards/resources/page/reducers/index'
import { types } from '../../../../brave_rewards/resources/page/constants/rewards_types'
import { defaultState } from '../../../../brave_rewards/resources/page/storage'

describe('publishers reducer', () => {
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

      const expectedState: Rewards.State = { ...defaultState }
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
      const initialState = { ...defaultState }
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

      const expectedState: Rewards.State = { ...defaultState }
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
