/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import HttpsUpgradesControl, { Props } from '../../../../../brave_extension/extension/brave_extension/containers/advancedView/controls/httpsUpgradesControl'
import { BlockOptions } from '../../../../../brave_extension/extension/brave_extension/types/other/blockTypes'
import * as actionTypes from '../../../../../brave_extension/extension/brave_extension/constants/shieldsPanelTypes'
import { shallow } from 'enzyme'

const fakeProps: Props = {
  isBlockedListOpen: true,
  setBlockedListOpen: () => { return },
  hostname: 'brave.com',
  favicon: '',
  httpsRedirected: 0,
  httpUpgradableResources: 'allow',
  httpsRedirectedResources: [],
  httpsEverywhereToggled: (setting: BlockOptions) => ({ type: actionTypes.HTTPS_EVERYWHERE_TOGGLED, setting })
}

describe('AdvancedView HttpsUpgradesControl component', () => {
  const baseComponent = (props: Props) =>
    <HttpsUpgradesControl {...props} />

  it('renders the component', () => {
    const wrapper = shallow(baseComponent(fakeProps))
    const assertion = wrapper.find('#httpsUpgradesControl').length === 1
    expect(assertion).toBe(true)
  })

  describe('https everywhere control', () => {
    it('responds to the onChange event', () => {
      const value = { target: { value: true } }
      const onChangeConnectionsEncryptedSwitch = jest.spyOn(fakeProps, 'httpsEverywhereToggled')
      const newProps = Object.assign(fakeProps, {
        httpsEverywhereToggled: onChangeConnectionsEncryptedSwitch
      })

      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#connectionsEncrypted').simulate('change', value)
      expect(onChangeConnectionsEncryptedSwitch).toBeCalled()
    })

    it('can toggle ad control on', () => {
      const newProps = Object.assign(fakeProps, { httpUpgradableResources: 'block' })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#connectionsEncrypted').prop('checked')
      expect(assertion).toBe(true)
    })

    it('can toggle ad control off', () => {
      const newProps = Object.assign(fakeProps, { httpUpgradableResources: 'allow' })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#connectionsEncrypted').prop('checked')
      expect(assertion).toBe(false)
    })

    it('shows number of https redirected', () => {
      const newProps = Object.assign(fakeProps, { httpsRedirected: 13 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#connectionsEncryptedStat').props().children
      expect(assertion).toBe('13')
    })

    it('trim https redirected to 99+ if number is higher', () => {
      const newProps = Object.assign(fakeProps, { httpsRedirected: 123123123 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#connectionsEncryptedStat').props().children
      expect(assertion).toBe('99+')
    })
  })
})
