/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import 'mocha'
import * as React from 'react'
import * as assert from 'assert'
import BraveShieldsControls, { BraveShieldsControlsProps } from '../../../../app/components/braveShields/braveShieldsControls'
import { BlockOptions, BlockFPOptions, BlockCookiesOptions } from '../../../../app/types/other/blockTypes'
import * as actionTypes from '../../../../app/constants/shieldsPanelTypes'
import { shallow } from 'enzyme'
import * as sinon from 'sinon'

const fakeProps: BraveShieldsControlsProps = {
  controlsOpen: false,
  braveShields: 'allow',
  httpUpgradableResources: 'allow',
  ads: 'allow',
  trackers: 'allow',
  javascript: 'block',
  fingerprinting: 'block',
  cookies: 'block',
  noScriptInfo: {},
  blockAdsTrackers: (setting: BlockOptions) => {
    return {
      type: actionTypes.BLOCK_ADS_TRACKERS,
      setting
    }
  },
  controlsToggled: (setting: boolean) => {
    return {
      type: actionTypes.CONTROLS_TOGGLED,
      setting
    }
  },
  httpsEverywhereToggled: () => {
    return {
      type: actionTypes.HTTPS_EVERYWHERE_TOGGLED
    }
  },
  javascriptToggled: () => {
    return {
      type: actionTypes.JAVASCRIPT_TOGGLED
    }
  },
  blockFingerprinting: (setting:BlockFPOptions) => {
    return {
      type: actionTypes.BLOCK_FINGERPRINTING,
      setting 
    }
  },
  blockCookies: (setting: BlockCookiesOptions) => {
    return {
      type: actionTypes.BLOCK_COOKIES,
      setting
    }
  },
  allowScriptOriginsOnce: (origins: string[]) => {
    return {
      type: actionTypes.ALLOW_SCRIPT_ORIGINS_ONCE,
      origins
    }
  },
  changeNoScriptSettings: (origin: string) => {
    return {
      type: actionTypes.CHANGE_NO_SCRIPT_SETTINGS,
      origin
    }
  }
}

describe('BraveShieldsControls component', () => {
  const baseComponent = (props: BraveShieldsControlsProps) =>
    <BraveShieldsControls {...props} />

  it('renders the component', () => {
    const wrapper = shallow(baseComponent(fakeProps))
    const assertion = wrapper.find('#braveShieldsControls').length === 1
    assert.equal(assertion, true)
  })

  describe('advanced options content toggle', () => {
    it('advanced options responds to the onClick event', () => {
      const onClickAdvancedOptions = sinon.spy()
      const newProps = Object.assign(fakeProps, {
        controlsToggled: onClickAdvancedOptions
      })
      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#advancedControlsToggle').simulate('click')
      assert.equal(onClickAdvancedOptions.calledOnce, true)
    })

    it('can toggle on', () => {
      const newProps = Object.assign(fakeProps, { controlsOpen: true })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#advancedControlsToggle').prop('open')
      assert.equal(assertion, true)
    })

    it('can toggle off', () => {
    // start with shields off
    const newProps1 = Object.assign(fakeProps, { controlsOpen: true })
    const wrapper = shallow(baseComponent(newProps1))
    const assertion1 = wrapper.find('#advancedControlsToggle').prop('open')
    assert.equal(assertion1, true)
    // then turn it on
    const newProps2 = Object.assign(fakeProps, { controlsOpen: true })
    const wrapper2 = shallow(baseComponent(newProps2))
    const assertion2 = wrapper2.find('#advancedControlsToggle').prop('open')
    assert.equal(assertion2, true)
    })
  })

  describe('ad control', () => {
    it('responds to the onChange event', () => {
      const value = {target: { value: true }}
      const onChangeAdControlSelectOptions = sinon.spy()
      const newProps = Object.assign(fakeProps, {
        blockAdsTrackers: onChangeAdControlSelectOptions
      })

      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#shieldsControlsAdControl').simulate('change', value)
      assert.equal(onChangeAdControlSelectOptions.calledOnce, true)
    })

    it('becomes disabled if brave shields is set to `block`', () => {
      const newProps = Object.assign(fakeProps, { braveShields: 'block' })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#shieldsControlsAdControl').prop('disabled')
      assert.equal(assertion, true)
    })
  })

  describe('cookie control', () => {
    it('responds to the onChange event', () => {
      const value = {target: { value: true }}
      const onChangeCookiesControlSelectOptions = sinon.spy()
      const newProps = Object.assign(fakeProps, {
        blockCookies: onChangeCookiesControlSelectOptions
      })

      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#shieldsControlsCookieControl').simulate('change', value)
      assert.equal(onChangeCookiesControlSelectOptions.calledOnce, true)
    })

    it('becomes disabled if brave shields is set to `block`', () => {
      const newProps = Object.assign(fakeProps, { braveShields: 'block' })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper.find('#shieldsControlsCookieControl').prop('disabled')
      assert.equal(assertion, true)
    })
  })

  describe('fingerprinting protection', () => {
    it('responds to the onChange event', () => {
      const value = {target: { value: true }}
      const onChangeFingerprintingProtectionSelectOptions = sinon.spy()
      const newProps = Object.assign(fakeProps, {
        blockFingerprinting: onChangeFingerprintingProtectionSelectOptions
      })

      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#shieldsControlsFingerprintingProtection').simulate('change', value)
      assert.equal(onChangeFingerprintingProtectionSelectOptions.calledOnce, true)
    })

    it('becomes disabled if brave shields is set to `block`', () => {
      const newProps = Object.assign(fakeProps, { braveShields: 'block' })
      const wrapper = shallow(baseComponent(newProps))
      const assertion = wrapper
        .find('#shieldsControlsFingerprintingProtection').prop('disabled')
      assert.equal(assertion, true)
    })
  })

  describe('https everywhere', () => {
    it('responds to the onChange event', () => {
      const value = {target: { value: true }}
      const onChangeHttpsEverywhereToggle = sinon.spy()
      const newProps = Object.assign(fakeProps, {
        httpsEverywhereToggled: onChangeHttpsEverywhereToggle
      })

      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#httpsEverywhere').simulate('change', value)
      assert.equal(onChangeHttpsEverywhereToggle.calledOnce, true)
    })
  })

  describe('block scripts', () => {
    it('responds to the onChange event', () => {
      const value = {target: { checked: true }}
      const onToggleBlockScripts = sinon.spy()
      const newProps = Object.assign(fakeProps, {
        javascriptToggled: onToggleBlockScripts
      })

      const wrapper = shallow(baseComponent(newProps))
      wrapper.find('#blockScripts').simulate('change', value)
      assert.equal(onToggleBlockScripts.calledOnce, true)
    })
  })

  describe('block pishing/malware', () => {
    // TBD
  })
})
