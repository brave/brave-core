/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { shallow } from 'enzyme'
import { types } from '../../../brave_rewards/ui/constants/rewards_types'
import { rewardsInitialState } from '../../testData'
import {
  App,
  mapStateToProps,
  mapDispatchToProps
} from '../../../brave_rewards/ui/components/app'

describe('rewardsPage component', () => {
  describe('mapStateToProps', () => {
    it('should map the default state', () => {
      expect(mapStateToProps(rewardsInitialState)).toEqual({
        rewardsData: {
          walletCreated: false,
          walletCreateFailed: false,
          createdTimestamp: null,
          enabledMain: false,
          enabledAds: false,
          enabledContribute: false,
          firstLoad: null,
          contributionMinTime: 8000,
          contributionMinVisits: 1,
          contributionMonthly: 10,
          contributionNonVerified: true,
          contributionVideos: true,
          donationAbilityYT: true,
          donationAbilityTwitter: true
        }
      })
    })
  })

  describe('mapDispatchToProps', () => {
    it('should fire walletCreated', () => {
      const dispatch = jest.fn()

      mapDispatchToProps(dispatch).actions.onWalletCreated()
      expect(dispatch.mock.calls[0][0]).toEqual({
        type: types.WALLET_CREATED,
        meta: undefined,
        payload: undefined
      })
    })
    it('should fire walletCreateFailed', () => {
      const dispatch = jest.fn()

      mapDispatchToProps(dispatch).actions.onWalletCreateFailed()
      expect(dispatch.mock.calls[0][0]).toEqual({
        type: types.WALLET_CREATE_FAILED,
        meta: undefined,
        payload: undefined
      })
    })
  })

  describe('rewardsPage dumb component', () => {
    it('renders the component', () => {
      const wrapper = shallow(
        <App
          actions={{}}
          rewardsData={rewardsInitialState.rewardsData as Rewards.State}
        />
      )
      const assertion = wrapper.find('#rewardsPage')
      expect(assertion.length).toBe(1)
    })
  })
})
