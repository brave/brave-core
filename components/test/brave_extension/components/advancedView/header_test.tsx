/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import ShieldsHeader, { Props } from '../../../../brave_extension/extension/brave_extension/containers/advancedView/header'
import { BlockOptions } from '../../../../brave_extension/extension/brave_extension/types/other/blockTypes'
import * as actionTypes from '../../../../brave_extension/extension/brave_extension/constants/shieldsPanelTypes'
import { shallow } from 'enzyme'

const fakeProps: Props = {
  enabled: false,
  favicon: '',
  origin: 'https://brave.com',
  hostname: 'brave.com',
  isBlockedListOpen: true,
  shieldsToggled: (setting: BlockOptions) => ({ type: actionTypes.SHIELDS_TOGGLED, setting }),
  adsBlocked: 0,
  trackersBlocked: 0,
  httpsUpgrades: 0,
  scriptsBlocked: 0,
  fingerprintingBlocked: 0
}

describe('AdvancedView Header component', () => {
  const baseComponent = (props: Props) =>
    <ShieldsHeader {...props} />

  it('renders the component', () => {
    const wrapper = shallow(baseComponent(fakeProps))
    const assertion = wrapper.find('#braveShieldsHeader').length === 1
    expect(assertion).toBe(true)
  })

  it('shields toggle responds to the onChange event', () => {
    const value = { target: { checked: true } }
    const onToggleShields = jest.spyOn(fakeProps, 'shieldsToggled')
    const newProps = Object.assign(fakeProps, {
      shieldsToggled: onToggleShields
    })
    const wrapper = shallow(baseComponent(newProps))
    wrapper.find('#mainToggle').simulate('change', value)
    expect(onToggleShields).toBeCalled()
  })

  it('can toggle shields off', () => {
    const newProps = Object.assign(fakeProps, { enabled: false })
    const wrapper = shallow(baseComponent(newProps))
    const assertion = wrapper.find('#mainToggle').prop('checked')
    expect(assertion).toBe(false)
  })

  it('can toggle shields on', () => {
    // start with shields off
    const newProps1 = Object.assign(fakeProps, { enabled: false })
    const wrapper = shallow(baseComponent(newProps1))
    const assertion1 = wrapper.find('#mainToggle').prop('checked')
    expect(assertion1).toBe(false)
    // then turn it on
    const newProps2 = Object.assign(fakeProps, { enabled: true })
    const wrapper2 = shallow(baseComponent(newProps2))
    const assertion2 = wrapper2.find('#mainToggle').prop('checked')
    expect(assertion2).toBe(true)
  })

  it('displays the hostname', () => {
    const newProps = Object.assign(fakeProps, { hostname: 'https://brian-bondy-canada-do-te-karate.com' })
    const wrapper = shallow(baseComponent(newProps))
    const assertion = wrapper.find('#hostname').props().children
    expect(assertion).toBe('https://brian-bondy-canada-do-te-karate.com')
  })
})
