/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as React from 'react'
import * as assert from 'assert'
import ShieldsInterfaceControls, { Props } from '../../../../app/components/braveShields/interfaceControls'
import { BlockOptions } from '../../../../app/types/other/blockTypes'
import * as actionTypes from '../../../../app/constants/shieldsPanelTypes'
import { shallow } from 'enzyme'
import * as sinon from 'sinon'

const fakeProps: Props = {
  url: 'https://brave.com',
  hostname: 'brave.com',
  braveShields: 'allow',
  adsBlocked: 0,
  adsBlockedResources: [],
  ads: 'allow',
  blockAdsTrackers: (setting: BlockOptions) => ({ type: actionTypes.BLOCK_ADS_TRACKERS, setting }),
  trackers: 'allow',
  trackersBlocked: 0,
  trackersBlockedResources: [],
  httpsRedirected: 0,
  httpsRedirectedResources: [],
  httpUpgradableResources: 'allow',
  httpsEverywhereToggled: (setting: BlockOptions) => ({ type: actionTypes.HTTPS_EVERYWHERE_TOGGLED, setting })
}

describe('InterfaceControls component', () => {
  const baseComponent = (props: Props) =>
    <ShieldsInterfaceControls {...props} />

  it('renders the component', () => {
    const wrapper = shallow(baseComponent(fakeProps))
    const assertion = wrapper.find('#braveShieldsInterfaceControls').length === 1
    assert.equal(assertion, true)
  })

  describe('ad control', () => {
    it('responds to the onChange event', () => {
      const value = { target: { value: true } }
      const onChangeAdControlSelectOptions = sinon.spy()
      const newProps = Object.assign(fakeProps, {
        blockAdsTrackers: onChangeAdControlSelectOptions
      })

      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#blockAds').simulate('change', value)
      assert.equal(onChangeAdControlSelectOptions.calledOnce, true)
    })

    it('can toggle ad control on', () => {
      const newProps = Object.assign(fakeProps, { ads: 'block', trackers: 'block' })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockAds').prop('checked')
      assert.equal(assertion, true)
    })

    it('can toggle ad control off', () => {
      const newProps = Object.assign(fakeProps, { ads: 'allow', trackers: 'allow' })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockAds').prop('checked')
      assert.equal(assertion, false)
    })

    it('shows number of ads blocked', () => {
      const newProps = Object.assign(fakeProps, { adsBlocked: 13 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockAdsStat').props().children
      assert.equal(assertion, 13)
    })

    it('trim ads blocked to 99+ if number is higher', () => {
      const newProps = Object.assign(fakeProps, { adsBlocked: 123123123123123 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockAdsStat').props().children
      assert.equal(assertion, '99+')
    })
  })

  describe('https everywhere control', () => {
    it('responds to the onChange event', () => {
      const value = { target: { value: true } }
      const onChangeConnectionsEncryptedSwitch = sinon.spy()
      const newProps = Object.assign(fakeProps, {
        httpsEverywhereToggled: onChangeConnectionsEncryptedSwitch
      })

      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#connectionsEncrypted').simulate('change', value)
      assert.equal(onChangeConnectionsEncryptedSwitch.calledOnce, true)
    })

    it('can toggle ad control on', () => {
      const newProps = Object.assign(fakeProps, { httpUpgradableResources: 'block' })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#connectionsEncrypted').prop('checked')
      assert.equal(assertion, true)
    })

    it('can toggle ad control off', () => {
      const newProps = Object.assign(fakeProps, { httpUpgradableResources: 'allow' })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#connectionsEncrypted').prop('checked')
      assert.equal(assertion, false)
    })

    it('shows number of https redirected', () => {
      const newProps = Object.assign(fakeProps, { httpsRedirected: 13 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#connectionsEncryptedStat').props().children
      assert.equal(assertion, 13)
    })

    it('trim https redirected to 99+ if number is higher', () => {
      const newProps = Object.assign(fakeProps, { httpsRedirected: 123123123 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#connectionsEncryptedStat').props().children
      assert.equal(assertion, '99+')
    })
  })
})
