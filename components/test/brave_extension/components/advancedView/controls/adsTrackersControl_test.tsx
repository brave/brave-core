/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import AdsTrackersControl, { Props } from '../../../../../brave_extension/extension/brave_extension/containers/advancedView/controls/adsTrackersControl'
import { BlockOptions } from '../../../../../brave_extension/extension/brave_extension/types/other/blockTypes'
import * as actionTypes from '../../../../../brave_extension/extension/brave_extension/constants/shieldsPanelTypes'
import { shallow } from 'enzyme'

const fakeProps: Props = {
  isBlockedListOpen: true,
  setBlockedListOpen: () => { return },
  hostname: 'brave.com',
  favicon: '',
  ads: 'allow',
  adsBlocked: 0,
  adsBlockedResources: [],
  trackers: 'allow',
  trackersBlocked: 0,
  trackersBlockedResources: [],
  blockAdsTrackers: (setting: BlockOptions) => ({ type: actionTypes.BLOCK_ADS_TRACKERS, setting })
}

describe('AdvancedView AdsTrackersControl component', () => {
  const baseComponent = (props: Props) =>
    <AdsTrackersControl {...props} />

  it('renders the component', () => {
    const wrapper = shallow(baseComponent(fakeProps))
    const assertion = wrapper.find('#adsTrackersControl').length === 1
    expect(assertion).toBe(true)
  })

  describe('ad control', () => {
    it('responds to the onChange event', () => {
      const value = { target: { value: true } }
      const onChangeAdControlSelectOptions = jest.spyOn(fakeProps, 'blockAdsTrackers')
      const newProps = Object.assign(fakeProps, {
        blockAdsTrackers: onChangeAdControlSelectOptions
      })

      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#blockAds').simulate('change', value)
      expect(onChangeAdControlSelectOptions).toBeCalled()
    })

    it('can set ad control to aggressive', () => {
      const newProps = Object.assign(fakeProps, { ads: 'block', trackers: 'block', firstPartyCosmeticFiltering: true })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockAds').prop('value')
      expect(assertion).toBe('block')
    })

    it('can set ad control to standard', () => {
      const newProps = Object.assign(fakeProps, { ads: 'block', trackers: 'block', firstPartyCosmeticFiltering: false })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockAds').prop('value')
      expect(assertion).toBe('block_third_party')
    })

    it('can disable ad control', () => {
      const newProps = Object.assign(fakeProps, { ads: 'allow', trackers: 'allow', firstPartyCosmeticFiltering: false })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockAds').prop('value')
      expect(assertion).toBe('allow')
    })

    it('shows number of ads blocked', () => {
      const newProps = Object.assign(fakeProps, { adsBlocked: 13 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockAdsStat').props().children
      expect(assertion).toBe('13')
    })

    it('trim ads blocked to 99+ if number is higher', () => {
      const newProps = Object.assign(fakeProps, { adsBlocked: 123123123123123 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockAdsStat').props().children
      expect(assertion).toBe('99+')
    })
  })
})
