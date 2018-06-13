/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as React from 'react'
import * as assert from 'assert'
import BraveShieldsHeader, { BraveShieldsHeaderProps } from '../../../../app/components/braveShields/braveShieldsHeader'
import { BlockOptions } from '../../../../app/types/other/blockTypes';
import * as actionTypes from '../../../../app/constants/shieldsPanelTypes'
import { shallow } from 'enzyme'
import * as sinon from 'sinon'

const fakeProps: BraveShieldsHeaderProps = {
  hostname: 'brave.com',
  shieldsToggled: (setting: BlockOptions) => {
    return {
      type: actionTypes.SHIELDS_TOGGLED,
      setting
    }
  },
  braveShields: 'allow'
}

describe('BraveShieldsHeader component', () => {
  const baseComponent = (props: BraveShieldsHeaderProps) =>
    <BraveShieldsHeader {...props} />

  it('renders the component', () => {
    const wrapper = shallow(baseComponent(fakeProps))
    const assertion = wrapper.find('#braveShieldsHeader').length === 1
    assert.equal(assertion, true)
  })

  it('shields toggle responds to the onChange event', () => {
    const value = {target: { checked: true }}
    const onToggleShields = sinon.spy()
    const newProps = Object.assign(fakeProps, {
      shieldsToggled: onToggleShields
    })
    const wrapper = shallow(baseComponent(newProps))
    wrapper.find('#shieldsToggle').simulate('change', value)
    assert.equal(onToggleShields.calledOnce, true)
  })

  it('can toggle shields off', () => {
    const newProps = Object.assign(fakeProps, { braveShields: 'block' })
    const wrapper = shallow(baseComponent(newProps))
    const assertion = wrapper.find('#shieldsToggle').prop('checked')
    assert.equal(assertion, false)
  })

  it('can toggle shields on', () => {
    // start with shields off
    const newProps1 = Object.assign(fakeProps, { braveShields: 'block' })
    const wrapper = shallow(baseComponent(newProps1))
    const assertion1 = wrapper.find('#shieldsToggle').prop('checked')
    assert.equal(assertion1, false)
    // then turn it on
    const newProps2 = Object.assign(fakeProps, { braveShields: 'allow' })
    const wrapper2 = shallow(baseComponent(newProps2))
    const assertion2 = wrapper2.find('#shieldsToggle').prop('checked')
    assert.equal(assertion2, true)
  })

  it('displays the hostname', () => {
    const newProps = Object.assign(fakeProps, {
      hostname: 'https://brian-bondy-canada-do-te-karate.com'
    })
    const wrapper = shallow(baseComponent(newProps))
    const assertion = wrapper.find('#hostname').prop('text')
    assert.equal(assertion, 'https://brian-bondy-canada-do-te-karate.com')
  })
})