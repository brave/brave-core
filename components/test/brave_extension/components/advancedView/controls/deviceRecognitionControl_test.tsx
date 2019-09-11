/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import DeviceRecognitionControl, { Props } from '../../../../../brave_extension/extension/brave_extension/containers/advancedView/controls/deviceRecognitionControl'
import { BlockOptions } from '../../../../../brave_extension/extension/brave_extension/types/other/blockTypes'
import * as actionTypes from '../../../../../brave_extension/extension/brave_extension/constants/shieldsPanelTypes'
import { shallow } from 'enzyme'

const fakeProps: Props = {
  favicon: '',
  hostname: 'brave.com',
  isBlockedListOpen: true,
  setBlockedListOpen: () => { return },
  fingerprinting: 'allow',
  fingerprintingBlocked: 0,
  fingerprintingBlockedResources: [],
  blockFingerprinting: (setting: BlockOptions) => ({ type: actionTypes.BLOCK_FINGERPRINTING, setting })
}

describe('AdvancedView DeviceRecognitionControl component', () => {
  const baseComponent = (props: Props) =>
    <DeviceRecognitionControl {...props} />

  it('renders the component', () => {
    const wrapper = shallow(baseComponent(fakeProps))
    const assertion = wrapper.find('#deviceRecognitionControl').length === 1
    expect(assertion).toBe(true)
  })

  describe('DeviceRecognitionControl control', () => {
    it('responds to the onChange event', () => {
      const value = { target: { value: true } }
      const onChangeFingerprintingProtectionSelectOptions = jest.spyOn(fakeProps, 'blockFingerprinting')
      const newProps = Object.assign(fakeProps, {
        blockFingerprinting: onChangeFingerprintingProtectionSelectOptions
      })

      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#blockFingerprinting').simulate('change', value)
      expect(onChangeFingerprintingProtectionSelectOptions).toBeCalled()
    })

    it('shows number of fingerprinting attempts blocked', () => {
      const newProps = Object.assign(fakeProps, { fingerprintingBlocked: 13 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockFingerprintingStat').props().children
      expect(assertion).toBe('13')
    })

    it('trim number of fingerprinting to 99+ if number is higher', () => {
      const newProps = Object.assign(fakeProps, { fingerprintingBlocked: 123123123 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockFingerprintingStat').props().children
      expect(assertion).toBe('99+')
    })
  })
})
