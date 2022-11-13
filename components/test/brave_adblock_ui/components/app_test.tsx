// Copyright (c) 2018 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { shallow } from 'enzyme'
import { adblockInitialState } from '../../testData'
import {
  AdblockPage,
  mapStateToProps
} from '../../../brave_adblock_ui/components/app'

describe('adblockPage component', () => {
  describe('mapStateToProps', () => {
    it('should map the default state', () => {
      expect(mapStateToProps(adblockInitialState)).toEqual(adblockInitialState)
    })
  })

  describe('adblockPage dumb component', () => {
    it('renders the component', () => {
      const wrapper = shallow(
        <AdblockPage
          actions={{}}
          adblockData={adblockInitialState.adblockData as AdBlock.State}
        />
      )
      const assertion = wrapper.find('#adblockPage')
      expect(assertion.length).toBe(1)
    })
  })
})
