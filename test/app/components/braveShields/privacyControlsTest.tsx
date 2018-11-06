/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as React from 'react'
import * as assert from 'assert'
import ShieldsPrivacyControls, { Props } from '../../../../app/components/braveShields/privacyControls'
import { BlockJSOptions, BlockCookiesOptions, BlockFPOptions } from '../../../../app/types/other/blockTypes'
import * as actionTypes from '../../../../app/constants/shieldsPanelTypes'
import { shallow } from 'enzyme'
import * as sinon from 'sinon'

const fakeProps: Props = {
  url: 'https://brave.com/jobs',
  hostname: 'brave.com',
  origin: 'https://brave.com',
  braveShields: 'allow',
  javascript: 'block',
  javascriptBlocked: 0,
  blockJavaScript: (setting: BlockJSOptions) => ({ type: actionTypes.JAVASCRIPT_TOGGLED, setting }),
  javascriptBlockedResources: [],
  noScriptInfo: {},
  allowScriptOriginsOnce: (origins: Array<string>) => ({ type: actionTypes.ALLOW_SCRIPT_ORIGINS_ONCE, origins }),
  changeNoScriptSettings: (origin: string) => ({ type: actionTypes.CHANGE_NO_SCRIPT_SETTINGS, origin }),
  changeAllNoScriptSettings: (origin: string, shouldBlock: boolean) => ({ type: actionTypes.CHANGE_ALL_NO_SCRIPT_SETTINGS, origin, shouldBlock }),
  fingerprinting: 'block',
  fingerprintingBlocked: 0,
  blockFingerprinting: (setting: BlockFPOptions) => ({ type: actionTypes.BLOCK_FINGERPRINTING, setting }),
  fingerprintingBlockedResources: [],
  cookies: 'block',
  blockCookies: (setting: BlockCookiesOptions) => ({ type: actionTypes.BLOCK_COOKIES, setting })
}

describe('PrivacyControls component', () => {
  const baseComponent = (props: Props) =>
    <ShieldsPrivacyControls {...props} />

  it('renders the component', () => {
    const wrapper = shallow(baseComponent(fakeProps))
    const assertion = wrapper.find('#braveShieldsPrivacyControls').length === 1
    assert.equal(assertion, true)
  })

  describe('cookie control', () => {
    it('responds to the onChange event', () => {
      const value = { target: { value: true } }
      const onChangeCookiesControlSelectOptions = sinon.spy()
      const newProps = Object.assign(fakeProps, {
        blockCookies: onChangeCookiesControlSelectOptions
      })

      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#blockCookies').simulate('change', value)
      assert.equal(onChangeCookiesControlSelectOptions.calledOnce, true)
    })
  })

  describe('scripts control', () => {
    it('responds to the onChange event', () => {
      const value = { target: { value: true } }
      const onChangeScriptControlSelectOptions = sinon.spy()
      const newProps = Object.assign(fakeProps, {
        blockJavaScript: onChangeScriptControlSelectOptions
      })

      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#blockScripts').simulate('change', value)
      assert.equal(onChangeScriptControlSelectOptions.calledOnce, true)
    })

    it('shows number of scripts blocked', () => {
      const newProps = Object.assign(fakeProps, { javascriptBlocked: 13 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockScriptsStat').props().children
      assert.equal(assertion, 13)
    })

    it('trim number of scripts blocked to 99+ if number is higher', () => {
      const newProps = Object.assign(fakeProps, { javascriptBlocked: 123123123 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockScriptsStat').props().children
      assert.equal(assertion, '99+')
    })
  })

  describe('fingerprinting control', () => {
    it('responds to the onChange event', () => {
      const value = { target: { value: true } }
      const onChangeFingerprintingProtectionSelectOptions = sinon.spy()
      const newProps = Object.assign(fakeProps, {
        blockFingerprinting: onChangeFingerprintingProtectionSelectOptions
      })

      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#blockFingerprinting').simulate('change', value)
      assert.equal(onChangeFingerprintingProtectionSelectOptions.calledOnce, true)
    })

    it('shows number of fingerprinting attempts blocked', () => {
      const newProps = Object.assign(fakeProps, { fingerprintingBlocked: 13 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockFingerprintingStat').props().children
      assert.equal(assertion, 13)
    })

    it('trim number of fingerprinting to 99+ if number is higher', () => {
      const newProps = Object.assign(fakeProps, { fingerprintingBlocked: 123123123 })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#blockFingerprintingStat').props().children
      assert.equal(assertion, '99+')
    })
  })
})
