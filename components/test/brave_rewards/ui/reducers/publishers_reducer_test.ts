/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
/* global chrome */

import reducers from '../../../../brave_rewards/resources/ui/reducers/index'
import { types } from '../../../../brave_rewards/resources/ui/constants/rewards_types'
import { defaultState } from '../../../../brave_rewards/resources/ui/storage'
import { rewardsInitialState } from '../../../testData'

describe('publishers reducer', () => {
  describe('ON_CONTRIBUTE_LIST', () => {
    it('does not include excluded publishers in auto-contribute list', () => {
      const initialState = reducers(rewardsInitialState, {
        type: types.ON_NUM_EXCLUDED_SITES,
        payload: {
          excludedSitesInfo: {
            num: 1,
            publisherKey: 'brave.com'
          }
        }
      })

      const assertion = reducers(initialState, {
        type: types.ON_CONTRIBUTE_LIST,
        payload: {
          list: [
            { publisherKey: 'brave.com', percentage: 0, verified: true, excluded: 0, url: 'https://brave.com', name: 'brave.com', id: 'brave.com', provider: '', favicon: '' },
            { publisherKey: 'test.com', percentage: 0, verified: true, excluded: 0, url: 'https://test.com', name: 'test.com', id: 'test.com', provider: '', favicon: '' },
            { publisherKey: 'test2.com', percentage: 0, verified: true, excluded: 0, url: 'https://test2.com', name: 'test2.com', id: 'test2.com', provider: '', favicon: '' }
          ]
        }
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.numExcludedSites = 1
      expectedState.excluded = ['brave.com']
      expectedState.contributeLoad = true
      expectedState.autoContributeList = [
        { publisherKey: 'test.com', percentage: 0, verified: true, excluded: 0, url: 'https://test.com', name: 'test.com', id: 'test.com', provider: '', favicon: '' },
        { publisherKey: 'test2.com', percentage: 0, verified: true, excluded: 0, url: 'https://test2.com', name: 'test2.com', id: 'test2.com', provider: '', favicon: '' }
      ]

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })

  describe('ON_NUM_EXCLUDED_SITES', () => {
    it('adds a recently excluded publisher to state.excluded', () => {
      const assertion = reducers(undefined, {
        type: types.ON_NUM_EXCLUDED_SITES,
        payload: {
          excludedSitesInfo: {
            num: 1,
            publisherKey: 'brave.com'
          }
        }
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.numExcludedSites = 1
      expectedState.excluded = ['brave.com']

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })

    it('removes a recently restored publisher from state.excluded', () => {
      let testState = reducers(rewardsInitialState, {
        type: types.ON_NUM_EXCLUDED_SITES,
        payload: {
          excludedSitesInfo: {
            num: 1,
            publisherKey: 'test.com'
          }
        }
      })

      testState = reducers(testState, {
        type: types.ON_NUM_EXCLUDED_SITES,
        payload: {
          excludedSitesInfo: {
            num: 2,
            publisherKey: 'brave.com'
          }
        }
      })

      testState = reducers(testState, {
        type: types.ON_NUM_EXCLUDED_SITES,
        payload: {
          excludedSitesInfo: {
            num: 1,
            publisherKey: 'test.com'
          }
        }
      })

      const expectedState: Rewards.State = { ...defaultState }
      expectedState.numExcludedSites = 1
      expectedState.excluded = ['brave.com']

      expect(testState).toEqual({
        rewardsData: expectedState
      })
    })

    it('does not modify state when excludedSitesInfo is null', () => {
      const assertion = reducers(undefined, {
        type: types.ON_NUM_EXCLUDED_SITES,
        payload: {
          excludedSitesInfo: null
        }
      })

      const expectedState: Rewards.State = { ...defaultState }

      expect(assertion).toEqual({
        rewardsData: expectedState
      })
    })
  })
})
