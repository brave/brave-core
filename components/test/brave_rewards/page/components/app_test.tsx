/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { shallow } from 'enzyme'
import { rewardsInitialState } from '../../../testData'
import {
  App,
  mapStateToProps
} from '../../../../brave_rewards/resources/page/components/app'

describe('rewardsPage component', () => {
  describe('mapStateToProps', () => {
    it('should map the default state', () => {
      expect(mapStateToProps(rewardsInitialState)).toEqual(rewardsInitialState)
    })
  })

  describe('rewardsPage dumb component', () => {
    it('renders the component', () => {
      const wrapper = shallow(
        <App
          actions={{
            isInitialized: () => false
          }}
          rewardsData={rewardsInitialState.rewardsData as Rewards.State}
        />
      )
      const assertion = wrapper.find('#rewardsPage')
      expect(assertion.length).toBe(1)
    })
  })
})
