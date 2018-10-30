/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as React from 'react'
import * as assert from 'assert'
import ShieldsHeader, { Props } from '../../../../app/components/braveShields/header'
import { BlockOptions } from '../../../../app/types/other/blockTypes'
import * as actionTypes from '../../../../app/constants/shieldsPanelTypes'
import { shallow } from 'enzyme'
import * as sinon from 'sinon'

const fakeProps: Props = {
  tabData: {
    hostname: 'brave.com',
    origin: 'https://brave.com',
    url: 'https://brave.com',
    braveShields: 'allow',
    adsBlocked: 0,
    ads: 'block',
    adsBlockedResources: [],
    httpsRedirected: 0,
    httpUpgradableResources: 'block',
    httpsRedirectedResources: [],
    id: 0,
    javascript: 'block',
    javascriptBlocked: 0,
    javascriptBlockedResources: [],
    trackers: 'block',
    trackersBlocked: 0,
    trackersBlockedResources: [],
    fingerprinting: 'block',
    fingerprintingBlocked: 0,
    fingerprintingBlockedResources: [],
    cookies: 'block',
    noScriptInfo: {},
    controlsOpen: false
  },
  shieldsToggled: (setting: BlockOptions) => {
    return { type: actionTypes.SHIELDS_TOGGLED, setting }
  },
}

describe('ShieldsHeader component', () => {
  const baseComponent = (props: Props) =>
    <ShieldsHeader {...props} />

  it('renders the component', () => {
    const wrapper = shallow(baseComponent(fakeProps))
    const assertion = wrapper.find('#braveShieldsHeader').length === 1
    assert.equal(assertion, true)
  })

  it('shields toggle responds to the onChange event', () => {
    const value = { target: { checked: true } }
    const onToggleShields = sinon.spy()
    const newProps = Object.assign(fakeProps, {
      shieldsToggled: onToggleShields
    })
    const wrapper = shallow(baseComponent(newProps))
    wrapper.find('#mainToggle').simulate('change', value)
    assert.equal(onToggleShields.calledOnce, true)
  })

  it('can toggle shields off', () => {
    const newProps = Object.assign(fakeProps, { tabData: { braveShields: 'block' } })
    const wrapper = shallow(baseComponent(newProps))
    const assertion = wrapper.find('#mainToggle').prop('checked')
    assert.equal(assertion, false)
  })

  it('can toggle shields on', () => {
    // start with shields off
    const newProps1 = Object.assign(fakeProps, { tabData: { braveShields: 'block' } })
    const wrapper = shallow(baseComponent(newProps1))
    const assertion1 = wrapper.find('#mainToggle').prop('checked')
    assert.equal(assertion1, false)
    // then turn it on
    const newProps2 = Object.assign(fakeProps, { tabData: { braveShields: 'allow' } })
    const wrapper2 = shallow(baseComponent(newProps2))
    const assertion2 = wrapper2.find('#mainToggle').prop('checked')
    assert.equal(assertion2, true)
  })

  it('displays the hostname', () => {
    const newProps = Object.assign(fakeProps, {
      tabData: { hostname: 'https://brian-bondy-canada-do-te-karate.com' }
    })
    const wrapper = shallow(baseComponent(newProps))
    const assertion = wrapper.find('#hostname').props().children
    assert.equal(assertion, 'https://brian-bondy-canada-do-te-karate.com')
  })
})
