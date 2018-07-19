/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { shallow } from 'enzyme'
import { types } from '../../../brave_rewards_ui/constants/rewards_types'
import { rewardsInitialState } from '../../testData'
import {
  RewardsPage,
  mapStateToProps,
  mapDispatchToProps
} from '../../../brave_rewards_ui/components/app'

describe('rewardsPage component', () => {
  describe('mapStateToProps', () => {
    it('should map the default state', () => {
      expect(mapStateToProps(rewardsInitialState)).toEqual({
        rewardsData: {
          walletCreateFailed: false,
          walletCreated: false
        }
      })
    })
  })

  describe('mapDispatchToProps', () => {
    it('should fire walletCreated', () => {
      const dispatch = jest.fn()

      mapDispatchToProps(dispatch).actions.walletCreated()
      expect(dispatch.mock.calls[0][0]).toEqual({
        type: types.WALLET_CREATED,
        meta: undefined,
        payload: undefined
      })
    })
    it('should fire walletCreateFailed', () => {
      const dispatch = jest.fn()

      mapDispatchToProps(dispatch).actions.walletCreateFailed()
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
        <RewardsPage
          actions={{}}
          rewardsData={rewardsInitialState.rewardsData}
        />
      )
      const assertion = wrapper.find('#rewardsPage')
      expect(assertion.length).toBe(1)
    })
  })
})
